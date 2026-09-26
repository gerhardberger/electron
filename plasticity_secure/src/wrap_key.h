// Copyright (c) 2026 Plastic Software LLC.

#ifndef PLASTICITY_SECURE_SRC_WRAP_KEY_H_
#define PLASTICITY_SECURE_SRC_WRAP_KEY_H_

#include "electron/plasticity_secure/src/key_config.h"

#define DECODE_WRAP_KEY(output)                                                \
  do {                                                                         \
    uint64_t decode_state = plasticity::secure::kWrapKeySeed;                  \
    for (size_t runtime_index = 0; runtime_index < sizeof(uint64_t);           \
         ++runtime_index) {                                                    \
      decode_state ^= static_cast<uint64_t>(                                   \
                          plasticity::secure::kRuntimeId[runtime_index])        \
                      << (runtime_index * 8);                                  \
    }                                                                          \
    for (size_t decode_index = 0; decode_index < (output).size();              \
         ++decode_index) {                                                     \
      decode_state += 0x9e3779b97f4a7c15ULL + decode_index;                    \
      decode_state ^= decode_state >> 30;                                      \
      decode_state *= 0xbf58476d1ce4e5b9ULL;                                   \
      decode_state ^= decode_state >> 27;                                      \
      decode_state *= 0x94d049bb133111ebULL;                                   \
      decode_state ^= decode_state >> 31;                                      \
      (output)[decode_index] =                                                 \
          plasticity::secure::kEncodedWrapKey[decode_index] ^                  \
          static_cast<uint8_t>(decode_state >> ((decode_index % 8) * 8));      \
    }                                                                          \
  } while (false)

#endif  // PLASTICITY_SECURE_SRC_WRAP_KEY_H_
