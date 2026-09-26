#pragma once

#include <array>
#include <cstdint>
#include <node_api.h>

struct PlasticityHost {
    std::array<uint8_t, 32> nonce;
    uintptr_t context;
    void (*respond)(uintptr_t, const uint8_t *, uint8_t *);
};

using PlasticityInitializer = napi_value (*)(napi_env, napi_value, const PlasticityHost *);

// HMAC-SHA-512 over both nonces, the N-API environment, and the initializer.
// Expand inside the caller's protected region. sha512.h supplies the primitive.
#define PLASTICITY_HOST_PROOF(hostNonce, addonNonce, env, initializer, output)              \
    do {                                                                                 \
        volatile uint64_t mask[4] = {0x18a97fb16a3434f3ULL, 0x3cd40cbf3ce7a215ULL, 0xef4db394688b0be3ULL, 0x03d79aba81bfa2cbULL}; \
        const uint64_t encoded[4] = {0x0518763bcefabc36ULL, 0x9d8385655006484eULL, 0x4ebe01a15ea3b0f4ULL, 0xb7df8250e2ca1976ULL}; \
        std::array<uint8_t, 128> pad{};                                                   \
        for (size_t i = 0; i < pad.size(); ++i)                                           \
            pad[i] = i < 32 ? static_cast<uint8_t>((encoded[i / 8] ^ mask[i / 8]) >>      \
                                                  ((i % 8) * 8)) ^ 0x36 : 0x36;         \
        const std::array<uint64_t, 2> identity = {                                        \
            reinterpret_cast<uintptr_t>(env), reinterpret_cast<uintptr_t>(initializer)}; \
        const uint8_t domain[] = "Plasticity pk host 1";                                  \
        sha512_context hash;                                                             \
        sha512_init(&hash);                                                              \
        sha512_update(&hash, pad.data(), pad.size());                                     \
        sha512_update(&hash, domain, sizeof(domain));                                     \
        sha512_update(&hash, (hostNonce), 32);                                            \
        sha512_update(&hash, (addonNonce), 32);                                           \
        sha512_update(&hash, reinterpret_cast<const uint8_t *>(identity.data()),          \
                      sizeof(identity));                                                \
        std::array<uint8_t, 64> inner;                                                   \
        sha512_final(&hash, inner.data());                                               \
        for (uint8_t &byte : pad)                                                        \
            byte ^= 0x36 ^ 0x5c;                                                         \
        sha512_init(&hash);                                                              \
        sha512_update(&hash, pad.data(), pad.size());                                     \
        sha512_update(&hash, inner.data(), inner.size());                                 \
        sha512_final(&hash, (output));                                                   \
        volatile uint8_t *wipe = pad.data();                                             \
        for (size_t i = 0; i < pad.size(); ++i)                                           \
            wipe[i] = 0;                                                                \
        wipe = inner.data();                                                            \
        for (size_t i = 0; i < inner.size(); ++i)                                         \
            wipe[i] = 0;                                                                \
    } while (false)
