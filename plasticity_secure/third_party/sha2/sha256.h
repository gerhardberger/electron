// Copyright (c) 2026 Plastic Software LLC.
// Scalar rounds adapted from vendor/keygen/sha256 (LekKit, MIT); see LICENSE-LekKit.

#ifndef PLASTICITY_SHA256_H_
#define PLASTICITY_SHA256_H_

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif
void plasticity_sha256_blocks(uint32_t* state, const uint8_t* input, size_t blocks);
#ifdef __cplusplus
}
#endif

#define PLASTICITY_SHA256_ROTATE(value, bits) \
  (((value) >> (bits)) | ((value) << (32 - (bits))))

// Expand inside the protected caller. Only complete bulk blocks use native assembly.
#define PLASTICITY_SHA256(input, length, output)                                      \
  do {                                                                               \
    const uint8_t* sha_bytes = (input);                                               \
    const size_t sha_length = (length);                                               \
    uint32_t sha_state[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,           \
                            0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};          \
    if (sha_length >= 64)                                                            \
      plasticity_sha256_blocks(sha_state, sha_bytes, sha_length / 64);                 \
    uint8_t sha_tail[128] = {};                                                       \
    const size_t sha_remainder = sha_length % 64;                                      \
    for (size_t sha_i = 0; sha_i < sha_remainder; ++sha_i)                              \
      sha_tail[sha_i] = sha_bytes[sha_length - sha_remainder + sha_i];                  \
    sha_tail[sha_remainder] = 0x80;                                                   \
    const size_t sha_tail_length = sha_remainder < 56 ? 64 : 128;                      \
    const uint64_t sha_bits = (uint64_t)sha_length * 8;                                \
    for (size_t sha_i = 0; sha_i < 8; ++sha_i)                                         \
      sha_tail[sha_tail_length - 1 - sha_i] = (uint8_t)(sha_bits >> (8 * sha_i));        \
    static const uint32_t sha_constants[64] = {                                       \
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1,          \
      0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,          \
      0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786,          \
      0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,          \
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147,          \
      0x06ca6351, 0x14292967, 0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,          \
      0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b,          \
      0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,          \
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a,          \
      0x5b9cca4f, 0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,          \
      0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};                                \
    for (size_t sha_offset = 0; sha_offset < sha_tail_length; sha_offset += 64) {       \
      uint32_t sha_words[64];                                                        \
      uint32_t sha_work[8];                                                          \
      for (size_t sha_i = 0; sha_i < 16; ++sha_i) {                                    \
        const uint8_t* sha_word = sha_tail + sha_offset + 4 * sha_i;                   \
        sha_words[sha_i] = (uint32_t)sha_word[0] << 24 |                               \
                          (uint32_t)sha_word[1] << 16 |                               \
                          (uint32_t)sha_word[2] << 8 | sha_word[3];                    \
      }                                                                              \
      for (size_t sha_i = 16; sha_i < 64; ++sha_i) {                                  \
        const uint32_t sha_s0 = PLASTICITY_SHA256_ROTATE(sha_words[sha_i - 15], 7) ^    \
            PLASTICITY_SHA256_ROTATE(sha_words[sha_i - 15], 18) ^                      \
            (sha_words[sha_i - 15] >> 3);                                            \
        const uint32_t sha_s1 = PLASTICITY_SHA256_ROTATE(sha_words[sha_i - 2], 17) ^    \
            PLASTICITY_SHA256_ROTATE(sha_words[sha_i - 2], 19) ^                       \
            (sha_words[sha_i - 2] >> 10);                                            \
        sha_words[sha_i] = sha_words[sha_i - 16] + sha_s0 +                           \
                          sha_words[sha_i - 7] + sha_s1;                             \
      }                                                                              \
      for (size_t sha_i = 0; sha_i < 8; ++sha_i) sha_work[sha_i] = sha_state[sha_i];   \
      for (size_t sha_i = 0; sha_i < 64; ++sha_i) {                                   \
        const uint32_t sha_s1 = PLASTICITY_SHA256_ROTATE(sha_work[4], 6) ^             \
            PLASTICITY_SHA256_ROTATE(sha_work[4], 11) ^                               \
            PLASTICITY_SHA256_ROTATE(sha_work[4], 25);                                \
        const uint32_t sha_choice = (sha_work[4] & sha_work[5]) ^                      \
                                   (~sha_work[4] & sha_work[6]);                     \
        const uint32_t sha_t1 = sha_work[7] + sha_s1 + sha_choice +                    \
                               sha_constants[sha_i] + sha_words[sha_i];             \
        const uint32_t sha_s0 = PLASTICITY_SHA256_ROTATE(sha_work[0], 2) ^             \
            PLASTICITY_SHA256_ROTATE(sha_work[0], 13) ^                               \
            PLASTICITY_SHA256_ROTATE(sha_work[0], 22);                                \
        const uint32_t sha_majority = (sha_work[0] & sha_work[1]) ^                    \
            (sha_work[0] & sha_work[2]) ^ (sha_work[1] & sha_work[2]);                 \
        for (size_t sha_j = 7; sha_j > 0; --sha_j) sha_work[sha_j] = sha_work[sha_j-1]; \
        sha_work[4] += sha_t1;                                                       \
        sha_work[0] = sha_t1 + sha_s0 + sha_majority;                                 \
      }                                                                              \
      for (size_t sha_i = 0; sha_i < 8; ++sha_i) sha_state[sha_i] += sha_work[sha_i];  \
    }                                                                                \
    for (size_t sha_i = 0; sha_i < 8; ++sha_i) {                                      \
      (output)[4 * sha_i] = (uint8_t)(sha_state[sha_i] >> 24);                         \
      (output)[4 * sha_i + 1] = (uint8_t)(sha_state[sha_i] >> 16);                     \
      (output)[4 * sha_i + 2] = (uint8_t)(sha_state[sha_i] >> 8);                      \
      (output)[4 * sha_i + 3] = (uint8_t)sha_state[sha_i];                            \
    }                                                                                \
  } while (0)

#endif  // PLASTICITY_SHA256_H_
