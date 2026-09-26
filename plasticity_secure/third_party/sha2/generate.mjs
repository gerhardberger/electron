import { execFileSync } from 'node:child_process';
import { readFileSync, writeFileSync } from 'node:fs';
process.chdir(import.meta.dirname);
for (const bits of [256, 512]) {
  const output = `sha${bits}-blocks.asm`;
  execFileSync('perl', ['upstream/sha512-x86_64.pl', 'nasm', output], { stdio: 'inherit' });
  writeFileSync(output,
    `; Generated from OpenSSL 3.5.8; see LICENSE and README.md.\n` +
    `%define OPENSSL_ia32cap_P plasticity_sha2_cpu_caps\n` +
    `%define sha${bits}_block_data_order plasticity_sha${bits}_blocks\n` +
    readFileSync(output, 'utf8'));
}
