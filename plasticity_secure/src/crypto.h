// Copyright (c) 2026 Plastic Software LLC.

#ifdef UNSAFE_BUFFERS_BUILD
#pragma allow_unsafe_buffers
#endif

#ifndef PLASTICITY_SECURE_SRC_CRYPTO_H_
#define PLASTICITY_SECURE_SRC_CRYPTO_H_

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#if defined(_MSC_VER)
#define PLASTICITY_CRYPTO_INLINE __forceinline
#else
#define PLASTICITY_CRYPTO_INLINE inline __attribute__((always_inline))
#endif

#define PLASTICITY_WIPE(bytes)                                      \
  do {                                                             \
    const auto wipe_bytes = std::span<uint8_t>(bytes);               \
    volatile uint8_t* wipe_output = wipe_bytes.data();               \
    for (size_t wipe_index = 0; wipe_index < wipe_bytes.size();       \
         ++wipe_index)                                             \
      wipe_output[wipe_index] = 0;                                  \
  } while (false)

namespace plasticity::secure::crypto {

namespace internal {

inline constexpr std::array<uint8_t, 256> kAesSbox = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67,
    0x2b, 0xfe, 0xd7, 0xab, 0x76, 0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59,
    0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0, 0xb7,
    0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1,
    0x71, 0xd8, 0x31, 0x15, 0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05,
    0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75, 0x09, 0x83,
    0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29,
    0xe3, 0x2f, 0x84, 0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b,
    0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf, 0xd0, 0xef, 0xaa,
    0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c,
    0x9f, 0xa8, 0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc,
    0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2, 0xcd, 0x0c, 0x13, 0xec,
    0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19,
    0x73, 0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee,
    0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb, 0xe0, 0x32, 0x3a, 0x0a, 0x49,
    0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4,
    0xea, 0x65, 0x7a, 0xae, 0x08, 0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6,
    0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a, 0x70,
    0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9,
    0x86, 0xc1, 0x1d, 0x9e, 0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e,
    0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf, 0x8c, 0xa1,
    0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0,
    0x54, 0xbb, 0x16,
};

PLASTICITY_CRYPTO_INLINE uint32_t ReadBigEndian32(const uint8_t* data) {
  return static_cast<uint32_t>(data[0]) << 24 |
         static_cast<uint32_t>(data[1]) << 16 |
         static_cast<uint32_t>(data[2]) << 8 |
         static_cast<uint32_t>(data[3]);
}

PLASTICITY_CRYPTO_INLINE void WriteBigEndian32(uint8_t* output, uint32_t value) {
  output[0] = static_cast<uint8_t>(value >> 24);
  output[1] = static_cast<uint8_t>(value >> 16);
  output[2] = static_cast<uint8_t>(value >> 8);
  output[3] = static_cast<uint8_t>(value);
}

PLASTICITY_CRYPTO_INLINE void WriteBigEndian64(uint8_t* output, uint64_t value) {
  for (size_t index = 0; index < 8; ++index)
    output[index] = static_cast<uint8_t>(value >> ((7 - index) * 8));
}

PLASTICITY_CRYPTO_INLINE uint8_t AesXtime(uint8_t value) {
  return static_cast<uint8_t>((value << 1) ^
                              ((value >> 7) * static_cast<uint8_t>(0x1b)));
}

PLASTICITY_CRYPTO_INLINE std::array<uint8_t, 240> ExpandAes256Key(
    std::span<const uint8_t, 32> key) {
  std::array<uint8_t, 240> expanded;
  std::ranges::copy(key, expanded.begin());
  size_t generated = key.size();
  uint8_t rcon = 1;
  std::array<uint8_t, 4> temporary;
  while (generated < expanded.size()) {
    std::ranges::copy(expanded.begin() + generated - 4,
                      expanded.begin() + generated, temporary.begin());
    if (generated % key.size() == 0) {
      const uint8_t first = temporary[0];
      temporary[0] = kAesSbox[temporary[1]] ^ rcon;
      temporary[1] = kAesSbox[temporary[2]];
      temporary[2] = kAesSbox[temporary[3]];
      temporary[3] = kAesSbox[first];
      rcon = AesXtime(rcon);
    } else if (generated % key.size() == 16) {
      for (uint8_t& value : temporary)
        value = kAesSbox[value];
    }
    for (size_t index = 0; index < temporary.size(); ++index) {
      expanded[generated] = expanded[generated - key.size()] ^ temporary[index];
      ++generated;
    }
  }
  PLASTICITY_WIPE(temporary);
  return expanded;
}

PLASTICITY_CRYPTO_INLINE void AddAesRoundKey(
    std::array<uint8_t, 16>* state,
    const std::array<uint8_t, 240>& expanded,
    size_t round) {
  for (size_t index = 0; index < state->size(); ++index)
    (*state)[index] ^= expanded[round * state->size() + index];
}

PLASTICITY_CRYPTO_INLINE void ShiftAesRows(std::array<uint8_t, 16>* state) {
  const std::array<uint8_t, 16> input = *state;
  (*state)[0] = input[0];
  (*state)[1] = input[5];
  (*state)[2] = input[10];
  (*state)[3] = input[15];
  (*state)[4] = input[4];
  (*state)[5] = input[9];
  (*state)[6] = input[14];
  (*state)[7] = input[3];
  (*state)[8] = input[8];
  (*state)[9] = input[13];
  (*state)[10] = input[2];
  (*state)[11] = input[7];
  (*state)[12] = input[12];
  (*state)[13] = input[1];
  (*state)[14] = input[6];
  (*state)[15] = input[11];
}

PLASTICITY_CRYPTO_INLINE void MixAesColumns(std::array<uint8_t, 16>* state) {
  for (size_t column = 0; column < 4; ++column) {
    uint8_t* values = state->data() + column * 4;
    const uint8_t sum = values[0] ^ values[1] ^ values[2] ^ values[3];
    const uint8_t first = values[0];
    values[0] ^= sum ^ AesXtime(values[0] ^ values[1]);
    values[1] ^= sum ^ AesXtime(values[1] ^ values[2]);
    values[2] ^= sum ^ AesXtime(values[2] ^ values[3]);
    values[3] ^= sum ^ AesXtime(values[3] ^ first);
  }
}

PLASTICITY_CRYPTO_INLINE std::array<uint8_t, 16> EncryptAes256Block(
    std::span<const uint8_t, 16> input,
    const std::array<uint8_t, 240>& expanded) {
  std::array<uint8_t, 16> state;
  std::ranges::copy(input, state.begin());
  AddAesRoundKey(&state, expanded, 0);
  for (size_t round = 1; round < 14; ++round) {
    for (uint8_t& value : state)
      value = kAesSbox[value];
    ShiftAesRows(&state);
    MixAesColumns(&state);
    AddAesRoundKey(&state, expanded, round);
  }
  for (uint8_t& value : state)
    value = kAesSbox[value];
  ShiftAesRows(&state);
  AddAesRoundKey(&state, expanded, 14);
  return state;
}

PLASTICITY_CRYPTO_INLINE void MultiplyGhash(std::array<uint8_t, 16>* value,
                                 std::span<const uint8_t, 16> hash_key) {
  uint64_t factor_high =
      static_cast<uint64_t>(ReadBigEndian32(hash_key.data())) << 32 |
      ReadBigEndian32(hash_key.data() + 4);
  uint64_t factor_low =
      static_cast<uint64_t>(ReadBigEndian32(hash_key.data() + 8)) << 32 |
      ReadBigEndian32(hash_key.data() + 12);
  uint64_t product_high = 0;
  uint64_t product_low = 0;
  for (size_t byte = 0; byte < value->size(); ++byte) {
    for (uint8_t mask = 0x80; mask != 0; mask >>= 1) {
      const uint64_t selected =
          uint64_t{0} - (((*value)[byte] & mask) != 0);
      product_high ^= factor_high & selected;
      product_low ^= factor_low & selected;
      const uint64_t reduction = uint64_t{0} - (factor_low & 1);
      factor_low = (factor_low >> 1) | (factor_high << 63);
      factor_high = (factor_high >> 1) ^
          (uint64_t{0xe100000000000000} & reduction);
    }
  }
  WriteBigEndian64(value->data(), product_high);
  WriteBigEndian64(value->data() + 8, product_low);
  PLASTICITY_WIPE((std::span<uint8_t>(
      reinterpret_cast<uint8_t*>(&factor_high), sizeof(factor_high))));
  PLASTICITY_WIPE((std::span<uint8_t>(
      reinterpret_cast<uint8_t*>(&factor_low), sizeof(factor_low))));
}

PLASTICITY_CRYPTO_INLINE void UpdateGhash(std::array<uint8_t, 16>* state,
                               std::span<const uint8_t, 16> hash_key,
                               std::span<const uint8_t> input) {
  size_t offset = 0;
  while (offset < input.size()) {
    const size_t length = std::min<size_t>(16, input.size() - offset);
    for (size_t index = 0; index < length; ++index)
      (*state)[index] ^= input[offset + index];
    MultiplyGhash(state, hash_key);
    offset += length;
  }
}

PLASTICITY_CRYPTO_INLINE void IncrementGcmCounter(std::array<uint8_t, 16>* counter) {
  uint32_t value = ReadBigEndian32(counter->data() + 12);
  WriteBigEndian32(counter->data() + 12, value + 1);
}

}  // namespace internal

PLASTICITY_CRYPTO_INLINE std::array<uint8_t, 16> Aes256GcmDecrypt(
    std::span<const uint8_t, 32> key,
    std::span<const uint8_t, 12> nonce,
    std::span<const uint8_t> ciphertext,
    std::span<const uint8_t> associated_data,
    std::span<uint8_t> plaintext) {
  std::array<uint8_t, 240> expanded = internal::ExpandAes256Key(key);
  const std::array<uint8_t, 16> zero = {};
  std::array<uint8_t, 16> hash_key =
      internal::EncryptAes256Block(zero, expanded);
  std::array<uint8_t, 16> ghash = {};
  internal::UpdateGhash(&ghash, hash_key, associated_data);
  internal::UpdateGhash(&ghash, hash_key, ciphertext);
  std::array<uint8_t, 16> lengths = {};
  internal::WriteBigEndian64(lengths.data(), associated_data.size() * 8);
  internal::WriteBigEndian64(lengths.data() + 8, ciphertext.size() * 8);
  internal::UpdateGhash(&ghash, hash_key, lengths);

  std::array<uint8_t, 16> initial_counter = {};
  std::ranges::copy(nonce, initial_counter.begin());
  initial_counter[15] = 1;
  std::array<uint8_t, 16> tag_mask =
      internal::EncryptAes256Block(initial_counter, expanded);
  std::array<uint8_t, 16> authentication;
  for (size_t index = 0; index < authentication.size(); ++index)
    authentication[index] = ghash[index] ^ tag_mask[index];

  std::array<uint8_t, 16> counter = initial_counter;
  size_t offset = 0;
  while (offset < ciphertext.size()) {
    internal::IncrementGcmCounter(&counter);
    std::array<uint8_t, 16> stream =
        internal::EncryptAes256Block(counter, expanded);
    const size_t length = std::min<size_t>(16, ciphertext.size() - offset);
    for (size_t index = 0; index < length; ++index)
      plaintext[offset + index] = ciphertext[offset + index] ^ stream[index];
    PLASTICITY_WIPE(stream);
    offset += length;
  }

  PLASTICITY_WIPE(expanded);
  PLASTICITY_WIPE(hash_key);
  PLASTICITY_WIPE(ghash);
  PLASTICITY_WIPE(lengths);
  PLASTICITY_WIPE(initial_counter);
  PLASTICITY_WIPE(tag_mask);
  PLASTICITY_WIPE(counter);
  return authentication;
}

}  // namespace plasticity::secure::crypto

#undef PLASTICITY_CRYPTO_INLINE

#endif  // PLASTICITY_SECURE_SRC_CRYPTO_H_
