# Private SHA-2

OpenSSL 3.5.8 `crypto/sha/asm/sha512-x86_64.pl` and
`crypto/perlasm/x86_64-xlate.pl`, plus `crypto/sha/asm/sha512-armv8.pl`,
`crypto/perlasm/arm-xlate.pl` and `crypto/arm_arch.h`, Apache-2.0.
Regenerate Windows x64 and macOS/Linux x64/ARM64 assembly with
`CC=clang node generate.mjs` (Perl and Clang required).
Symbols are renamed to keep these routines independent of BoringSSL/OpenSSL.

`cpu.cc` enables SHA-256 instructions or AVX2/BMI1/BMI2 when supported by the CPU
and OS. Otherwise the assembly uses its scalar implementation. The capability
bits select equivalent arithmetic implementations, never authorization.
ARM64 similarly selects SHA-256/SHA-512 instructions from CPU capabilities.
Upstream source: https://github.com/openssl/openssl/tree/openssl-3.5.8/crypto

SHA-512 retains its protected initialization/finalization and uses optimized
assembly for bulk blocks. `PLASTICITY_SHA256` expands initialization, final
padding/block rounds, and digest construction directly inside the protected
caller. Its complete input blocks use optimized assembly.

The SHA-256 scalar rounds derive from Plasticity's `vendor/keygen/sha256`,
Copyright 2020 LekKit, MIT (`LICENSE-LekKit`).
