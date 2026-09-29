// Copyright (c) 2026 Plastic Software LLC.
#ifndef PLASTICITY_SECURE_SRC_AES_BLOCK_H_
#define PLASTICITY_SECURE_SRC_AES_BLOCK_H_

#if defined(__aarch64__)
#include <arm_neon.h>
#define PLASTICITY_BLOCK uint8x16_t
#define PLASTICITY_LOAD(bytes) vld1q_u8(reinterpret_cast<const uint8_t*>(bytes))
#define PLASTICITY_STORE(bytes, block) vst1q_u8(reinterpret_cast<uint8_t*>(bytes), block)
#define PLASTICITY_XOR veorq_u8
#define PLASTICITY_AND vandq_u8
#define PLASTICITY_AES_ROUND(block, key) \
  veorq_u8(vaesmcq_u8(vaeseq_u8(block, vdupq_n_u8(0))), key)
#define PLASTICITY_LENGTHS(high, low) \
  vreinterpretq_u8_u64(vcombine_u64(vcreate_u64(low), vcreate_u64(high)))
#define PLASTICITY_EQUAL(first, second) \
  (vminvq_u8(vceqq_u8(first, second)) == 0xff)
#elif defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define PLASTICITY_BLOCK __m128i
#define PLASTICITY_LOAD(bytes) _mm_loadu_si128(reinterpret_cast<const __m128i*>(bytes))
#define PLASTICITY_STORE(bytes, block) _mm_storeu_si128(reinterpret_cast<__m128i*>(bytes), block)
#define PLASTICITY_XOR _mm_xor_si128
#define PLASTICITY_AND _mm_and_si128
#define PLASTICITY_AES_ROUND _mm_aesenc_si128
#define PLASTICITY_LENGTHS _mm_set_epi64x
#define PLASTICITY_EQUAL(first, second) \
  (_mm_movemask_epi8(_mm_cmpeq_epi8(first, second)) == 0xffff)
#else
#error Unsupported Plasticity security architecture
#endif

#endif  // PLASTICITY_SECURE_SRC_AES_BLOCK_H_
