// Copyright (c) 2026 Plastic Software LLC.

#include <array>
#include <cstdint>
#include <intrin.h>

// Only performance dispatch: SHA-256 instructions, AVX2/BMI, or scalar assembly.
extern "C" const std::array<uint32_t, 4> plasticity_sha2_cpu_caps = [] {
  std::array<uint32_t, 4> caps = {};
  int registers[4];
  __cpuid(registers, 0);
  const int maximum_leaf = registers[0];
  __cpuid(registers, 1);
  const bool sse41 = (registers[2] & (1 << 19)) != 0;
  const bool ssse3 = (registers[2] & (1 << 9)) != 0;
  const bool avx_enabled = (registers[2] & (1 << 27)) != 0 &&
                          (registers[2] & (1 << 28)) != 0 &&
                          (_xgetbv(0) & 6) == 6;
  if (maximum_leaf >= 7) {
    __cpuidex(registers, 7, 0);
    if (sse41 && ssse3)
      caps[2] |= registers[1] & (1U << 29);
    if (avx_enabled)
      caps[2] |= registers[1] & 0x128;
  }
  return caps;
}();
