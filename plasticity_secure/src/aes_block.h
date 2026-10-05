// Copyright (c) 2026 Plastic Software LLC.
#ifndef PLASTICITY_SECURE_SRC_AES_BLOCK_H_
#define PLASTICITY_SECURE_SRC_AES_BLOCK_H_

#include "crypto.h"

#if defined(__aarch64__)
#include <arm_neon.h>
extern "C" const uint32_t plasticity_sha2_arm_caps;
#define PLASTICITY_HAS_HARDWARE_AES ((plasticity_sha2_arm_caps & (1U << 2)) != 0)
#define PLASTICITY_BLOCK uint8x16_t
#define PLASTICITY_LOAD(bytes) vld1q_u8(reinterpret_cast<const uint8_t*>(bytes))
#define PLASTICITY_STORE(bytes, block) vst1q_u8(reinterpret_cast<uint8_t*>(bytes), block)
#define PLASTICITY_XOR veorq_u8
#define PLASTICITY_AND vandq_u8
#define PLASTICITY_HARDWARE_AES_ROUND(block, key, output)       \
  do {                                                          \
    (output) = (block);                                         \
    const PLASTICITY_BLOCK aes_zero = vdupq_n_u8(0);            \
    __asm__(".arch_extension crypto\n\taese %0.16b, %1.16b\n\t" \
            "aesmc %0.16b, %0.16b"                              \
            : "+w"(output) : "w"(aes_zero));                    \
    (output) = PLASTICITY_XOR((output), (key));                 \
  } while (false)
#define PLASTICITY_LENGTHS(high, low) \
  vreinterpretq_u8_u64(vcombine_u64(vcreate_u64(low), vcreate_u64(high)))
#define PLASTICITY_EQUAL(first, second) \
  (vminvq_u8(vceqq_u8(first, second)) == 0xff)
#define PLASTICITY_WIPE_BLOCK(bytes, zero)  \
  __asm__ __volatile__("stp %q1, %q1, [%0]" \
                       : : "r"(bytes), "w"(zero) : "memory")
#elif defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
extern "C" const std::array<uint32_t, 4> plasticity_sha2_cpu_caps;
#define PLASTICITY_HAS_HARDWARE_AES ((plasticity_sha2_cpu_caps[1] & (1U << 25)) != 0)
#define PLASTICITY_BLOCK __m128i
#define PLASTICITY_LOAD(bytes) _mm_loadu_si128(reinterpret_cast<const __m128i*>(bytes))
#define PLASTICITY_STORE(bytes, block) _mm_storeu_si128(reinterpret_cast<__m128i*>(bytes), block)
#define PLASTICITY_XOR _mm_xor_si128
#define PLASTICITY_AND _mm_and_si128
#define PLASTICITY_HARDWARE_AES_ROUND(block, key, output) \
  do {                                                    \
    (output) = (block);                                   \
    __asm__("aesenc %1, %0" : "+x"(output) : "x"(key));   \
  } while (false)
#define PLASTICITY_LENGTHS _mm_set_epi64x
#define PLASTICITY_EQUAL(first, second) \
  (_mm_movemask_epi8(_mm_cmpeq_epi8(first, second)) == 0xffff)
#define PLASTICITY_WIPE_BLOCK(bytes, zero)                    \
  __asm__ __volatile__("movdqu %1, (%0)\n\tmovdqu %1, 16(%0)" \
                       : : "r"(bytes), "x"(zero) : "memory")
#else
#error Unsupported Plasticity security architecture
#endif

#define PLASTICITY_AES_ROUND(block, key, output)                               \
  do {                                                                         \
    const PLASTICITY_BLOCK aes_input = (block);                                \
    const PLASTICITY_BLOCK aes_key = (key);                                    \
    if (hardware_aes) {                                                        \
      PLASTICITY_HARDWARE_AES_ROUND(aes_input, aes_key, output);               \
    } else {                                                                   \
      std::array<uint8_t, 16> aes_bytes;                                       \
      std::array<uint8_t, 16> aes_shifted;                                     \
      PLASTICITY_STORE(aes_bytes.data(), aes_input);                           \
      for (size_t aes_column = 0; aes_column < 4; ++aes_column)                \
        for (size_t aes_row = 0; aes_row < 4; ++aes_row)                       \
          aes_shifted[4 * aes_column + aes_row] =                              \
              plasticity::secure::crypto::internal::kAesSbox[                  \
                  aes_bytes[4 * ((aes_column + aes_row) % 4) + aes_row]];      \
      for (size_t aes_column = 0; aes_column < 4; ++aes_column) {              \
        uint8_t* aes_values = aes_shifted.data() + 4 * aes_column;             \
        const uint8_t aes_sum =                                                \
            aes_values[0] ^ aes_values[1] ^ aes_values[2] ^ aes_values[3];     \
        const uint8_t aes_first = aes_values[0];                               \
        for (size_t aes_row = 0; aes_row < 4; ++aes_row) {                     \
          const uint8_t aes_difference = aes_values[aes_row] ^                 \
              (aes_row == 3 ? aes_first : aes_values[aes_row + 1]);            \
          aes_values[aes_row] ^= aes_sum ^                                     \
              static_cast<uint8_t>((aes_difference << 1) ^                     \
                  ((aes_difference >> 7) * 0x1b));                             \
        }                                                                      \
      }                                                                        \
      (output) = PLASTICITY_XOR(PLASTICITY_LOAD(aes_shifted.data()), aes_key); \
    }                                                                          \
  } while (false)

#endif  // PLASTICITY_SECURE_SRC_AES_BLOCK_H_
