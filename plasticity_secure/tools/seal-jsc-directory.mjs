import fs from 'node:fs';
import os from 'node:os';
import path from 'node:path';
import { execFileSync } from 'node:child_process';

const [directory, ...signingArguments] = process.argv.slice(2);
if (!directory)
  throw new Error('usage: seal-jsc-directory.mjs DIRECTORY <create-asset-pack signing arguments>');

const entries = fs.readdirSync(directory, { recursive: true, withFileTypes: true });
const temporary = fs.mkdtempSync(path.join(os.tmpdir(), 'plasticity-jsc-'));
try {
  for (const entry of entries) {
    if (!entry.isFile() || !entry.name.endsWith('.jsc')) continue;
    const filename = path.join(entry.parentPath, entry.name);
    const bytecode = fs.readFileSync(filename);
    if (bytecode.length < 24 || bytecode[2] !== 0xde || bytecode[3] !== 0xc0)
      throw new Error(`Expected freshly compiled V8 bytecode: ${filename}`);
    const length = bytecode.readUInt32LE(8) & 0x1fffffff;
    fs.writeFileSync(path.join(temporary, 'input.jsc'), bytecode);
    fs.writeFileSync(path.join(temporary, 'source'), Buffer.alloc(length, 0x20));
    execFileSync(process.execPath, [path.join(import.meta.dirname, 'create-asset-pack.mjs'),
      ...signingArguments, '--jsc', path.join(temporary, 'input.jsc'),
      '--source', path.join(temporary, 'source'), '--output', path.join(temporary, 'sealed.jsc')], { stdio: 'inherit' });
    fs.copyFileSync(path.join(temporary, 'sealed.jsc'), filename);

    if (entry.name === 'index.jsc' && path.basename(entry.parentPath).endsWith('.compiled')) {
      const name = path.basename(entry.parentPath).slice(0, -'.compiled'.length);
      const loader = path.join(entry.parentPath, '..', name, 'index.js');
      fs.writeFileSync(loader, `(() => {
  const Module = require('node:module');
  const fs = require('node:fs');
  const path = require('node:path');
  const vm = require('node:vm');
  require('node:v8').setFlagsFromString('--no-lazy --no-flush-bytecode');
  Module._extensions['.jsc'] = function(module, filename) {
    const wrapper = new vm.Script('', { cachedData: fs.readFileSync(filename), filename }).runInThisContext();
    wrapper.call(module.exports, module.exports, Module.createRequire(filename), module, filename, path.dirname(filename), process, global);
  };
  require(${JSON.stringify(`../${name}.compiled/index.jsc`)});
})();
`);
    }
    console.log(`Sealed ${path.relative(directory, filename)}`);
  }
} finally {
  fs.rmSync(temporary, { recursive: true });
}
