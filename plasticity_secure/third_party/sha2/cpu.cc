// Copyright (c) 2026 Plastic Software LLC.

#include <array>
#include <cstdint>
#if defined(__aarch64__)
#include "arm_arch.h"
#if defined(__APPLE__)
#include <sys/sysctl.h>
#else
#include <sys/auxv.h>
#include <asm/hwcap.h>
#endif

extern "C" const uint32_t plasticity_sha2_arm_caps = [] {
  uint32_t caps = ARMV7_NEON;
#if defined(__APPLE__)
  caps |= ARMV8_SHA256 | ARMV8_AES;
  int sha512 = 0;
  size_t size = sizeof(sha512);
  if (sysctlbyname("hw.optional.armv8_2_sha512", &sha512, &size, nullptr, 0) == 0 && sha512)
    caps |= ARMV8_SHA512;
#else
  const unsigned long hardware = getauxval(AT_HWCAP);
  if (hardware & HWCAP_AES)
    caps |= ARMV8_AES;
  if (hardware & HWCAP_SHA2)
    caps |= ARMV8_SHA256;
  if (hardware & HWCAP_SHA512)
    caps |= ARMV8_SHA512;
#endif
  return caps;
}();
#else
#if defined(_WIN32)
#include <intrin.h>
#else
#include <cpuid.h>
#endif

// Only performance dispatch: SHA-256 instructions, AVX2/BMI, or scalar assembly.
extern "C" const std::array<uint32_t, 4> plasticity_sha2_cpu_caps = [] {
  std::array<uint32_t, 4> caps = {};
  int registers[4];
#if defined(_WIN32)
  __cpuid(registers, 0);
#else
  __cpuid_count(0, 0, registers[0], registers[1], registers[2], registers[3]);
#endif
  const int maximum_leaf = registers[0];
#if defined(_WIN32)
  __cpuid(registers, 1);
#else
  __cpuid_count(1, 0, registers[0], registers[1], registers[2], registers[3]);
#endif
  const bool sse41 = (registers[2] & (1 << 19)) != 0;
  caps[1] |= registers[2] & (1U << 25);
  const bool ssse3 = (registers[2] & (1 << 9)) != 0;
  bool avx_enabled = (registers[2] & (1 << 27)) != 0 &&
                     (registers[2] & (1 << 28)) != 0;
  if (avx_enabled) {
#if defined(_WIN32)
    avx_enabled = (_xgetbv(0) & 6) == 6;
#else
    uint32_t low, high;
    __asm__ volatile("xgetbv" : "=a"(low), "=d"(high) : "c"(0));
    avx_enabled = (low & 6) == 6;
#endif
  }
  if (maximum_leaf >= 7) {
#if defined(_WIN32)
    __cpuidex(registers, 7, 0);
#else
    __cpuid_count(7, 0, registers[0], registers[1], registers[2], registers[3]);
#endif
    if (sse41 && ssse3)
      caps[2] |= registers[1] & (1U << 29);
    if (avx_enabled)
      caps[2] |= registers[1] & 0x128;
  }
  return caps;
}();
#endif
