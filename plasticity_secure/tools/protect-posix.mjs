import { existsSync, readFileSync, writeFileSync } from 'node:fs';
import { resolve, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { execFileSync, spawnSync } from 'node:child_process';

const [virtualizer, input, output] = process.argv.slice(2).map(path => resolve(path));
if (!virtualizer || !input || !output)
  throw new Error('Usage: node protect-posix.mjs <Virtualizer.exe> <input binary> <new output binary>');
if (existsSync(output))
  throw new Error(`Output already exists: ${output}`);

const bytes = readFileSync(input);
const magic = bytes.readUInt32LE(0);
if (magic !== 0x464c457f && magic !== 0xfeedfacf)
  throw new Error('Expected a Linux ELF or a single-architecture macOS Mach-O binary');

// The two SDK forms used by Electron: generic ARM64 and explicit TIGER WHITE x64.
const markers = [];
for (const prefix of ['eb10', '05000014']) {
  for (const id of [12, 13, 103, 503]) {
    const body = Buffer.alloc(8);
    body.writeUInt32LE(id);
    markers.push({ id, bytes: Buffer.concat([
      Buffer.from(prefix + '43562020', 'hex'), body, Buffer.from('43562020', 'hex'),
    ]) });
  }
}
let starts = 0;
let ends = 0;
for (const marker of markers) {
  for (let offset = bytes.indexOf(marker.bytes); offset !== -1;
       offset = bytes.indexOf(marker.bytes, offset + marker.bytes.length)) {
    if (marker.id === 12 || marker.id === 103) ++starts;
    else ++ends;
  }
}
if (!starts || starts !== ends)
  throw new Error(`Expected balanced Oreans markers; found ${starts} starts and ${ends} ends`);

const profile = resolve(dirname(fileURLToPath(import.meta.url)), 'protect-posix.cv');
const [profilePath, inputPath, outputPath] = [profile, input, output].map(path =>
  process.platform === 'win32' ? path : execFileSync('winepath', ['-w', path], { encoding: 'utf8' }).trim());
const args = ['/protect', profilePath, '/inputfile', inputPath, '/outputfile', outputPath, '/shareconsole', '/isolate'];
// Same Windows protector as Plasticity CI; Wine is required when run on POSIX.
const result = process.platform === 'win32'
  ? spawnSync(virtualizer, args, { encoding: 'utf8' })
  : spawnSync('wine', [virtualizer, ...args], { encoding: 'utf8' });
if (result.error) throw result.error;
const log = (result.stdout || '') + (result.stderr || '');
writeFileSync(output + '.log', log);
process.stdout.write(log);
if (result.status !== 0 || !/\b[1-9][0-9]* macros found\b/.test(log))
  throw new Error(`Oreans did not confirm protection (exit ${result.status}); see ${output}.log`);
const protectedBytes = readFileSync(output);
if (markers.some(marker => protectedBytes.includes(marker.bytes)))
  throw new Error('Oreans markers remain in the output');
console.log(`Protected ${starts} input regions; no input markers remain. Run the smoke test on the target OS.`);
