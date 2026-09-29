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

// SHA-512 rounds adapted from third_party/ed25519/sha512.c (LibTomCrypt).
// Four blocks, expanded wholly inside the caller's protected region.
// Volatile workspaces prevent extraction into native memcpy/memset helpers.
#define PLASTICITY_HOST_ROTATE(value, bits) (((value) >> (bits)) | ((value) << (64 - (bits))))
#define PLASTICITY_HOST_PROOF(hostNonce, addonNonce, env, initializer, output) \
do { \
    volatile uint64_t mask[4] = {0x18a97fb16a3434f3ULL, 0x3cd40cbf3ce7a215ULL, 0xef4db394688b0be3ULL, 0x03d79aba81bfa2cbULL}; \
    const uint64_t encoded[4] = {0x0518763bcefabc36ULL, 0x9d8385655006484eULL, 0x4ebe01a15ea3b0f4ULL, 0xb7df8250e2ca1976ULL}; \
    static const uint64_t constants[80] = { \
UINT64_C(0x428a2f98d728ae22), UINT64_C(0x7137449123ef65cd),  \
    UINT64_C(0xb5c0fbcfec4d3b2f), UINT64_C(0xe9b5dba58189dbbc), \
    UINT64_C(0x3956c25bf348b538), UINT64_C(0x59f111f1b605d019),  \
    UINT64_C(0x923f82a4af194f9b), UINT64_C(0xab1c5ed5da6d8118), \
    UINT64_C(0xd807aa98a3030242), UINT64_C(0x12835b0145706fbe),  \
    UINT64_C(0x243185be4ee4b28c), UINT64_C(0x550c7dc3d5ffb4e2), \
    UINT64_C(0x72be5d74f27b896f), UINT64_C(0x80deb1fe3b1696b1),  \
    UINT64_C(0x9bdc06a725c71235), UINT64_C(0xc19bf174cf692694), \
    UINT64_C(0xe49b69c19ef14ad2), UINT64_C(0xefbe4786384f25e3),  \
    UINT64_C(0x0fc19dc68b8cd5b5), UINT64_C(0x240ca1cc77ac9c65), \
    UINT64_C(0x2de92c6f592b0275), UINT64_C(0x4a7484aa6ea6e483),  \
    UINT64_C(0x5cb0a9dcbd41fbd4), UINT64_C(0x76f988da831153b5), \
    UINT64_C(0x983e5152ee66dfab), UINT64_C(0xa831c66d2db43210),  \
    UINT64_C(0xb00327c898fb213f), UINT64_C(0xbf597fc7beef0ee4), \
    UINT64_C(0xc6e00bf33da88fc2), UINT64_C(0xd5a79147930aa725),  \
    UINT64_C(0x06ca6351e003826f), UINT64_C(0x142929670a0e6e70), \
    UINT64_C(0x27b70a8546d22ffc), UINT64_C(0x2e1b21385c26c926),  \
    UINT64_C(0x4d2c6dfc5ac42aed), UINT64_C(0x53380d139d95b3df), \
    UINT64_C(0x650a73548baf63de), UINT64_C(0x766a0abb3c77b2a8),  \
    UINT64_C(0x81c2c92e47edaee6), UINT64_C(0x92722c851482353b), \
    UINT64_C(0xa2bfe8a14cf10364), UINT64_C(0xa81a664bbc423001), \
    UINT64_C(0xc24b8b70d0f89791), UINT64_C(0xc76c51a30654be30), \
    UINT64_C(0xd192e819d6ef5218), UINT64_C(0xd69906245565a910),  \
    UINT64_C(0xf40e35855771202a), UINT64_C(0x106aa07032bbd1b8), \
    UINT64_C(0x19a4c116b8d2d0c8), UINT64_C(0x1e376c085141ab53),  \
    UINT64_C(0x2748774cdf8eeb99), UINT64_C(0x34b0bcb5e19b48a8), \
    UINT64_C(0x391c0cb3c5c95a63), UINT64_C(0x4ed8aa4ae3418acb),  \
    UINT64_C(0x5b9cca4f7763e373), UINT64_C(0x682e6ff3d6b2b8a3), \
    UINT64_C(0x748f82ee5defb2fc), UINT64_C(0x78a5636f43172f60),  \
    UINT64_C(0x84c87814a1f0ab72), UINT64_C(0x8cc702081a6439ec), \
    UINT64_C(0x90befffa23631e28), UINT64_C(0xa4506cebde82bde9),  \
    UINT64_C(0xbef9a3f7b2c67915), UINT64_C(0xc67178f2e372532b), \
    UINT64_C(0xca273eceea26619c), UINT64_C(0xd186b8c721c0c207),  \
    UINT64_C(0xeada7dd6cde0eb1e), UINT64_C(0xf57d4f7fee6ed178), \
    UINT64_C(0x06f067aa72176fba), UINT64_C(0x0a637dc5a2c898a6),  \
    UINT64_C(0x113f9804bef90dae), UINT64_C(0x1b710b35131c471b), \
    UINT64_C(0x28db77f523047d84), UINT64_C(0x32caab7b40c72493),  \
    UINT64_C(0x3c9ebe0a15c9bebc), UINT64_C(0x431d67c49c100d4c), \
    UINT64_C(0x4cc5d4becb3e42b6), UINT64_C(0x597f299cfc657e2a),  \
    UINT64_C(0x5fcb6fab3ad6faec), UINT64_C(0x6c44198c4a475817) \
    }; \
    const uint64_t initial[8] = { \
        0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL, 0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL, \
        0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL, 0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL \
    }; \
    const uint64_t identity[2] = { \
        reinterpret_cast<uintptr_t>(env), reinterpret_cast<uintptr_t>(initializer) \
    }; \
    const uint8_t domain[] = "Plasticity pk host 1"; \
    volatile uint8_t message[256], inner[64]; \
    volatile uint64_t state[8], words[80], work[8]; \
    for (size_t pass = 0; pass < 2; ++pass) { \
        for (size_t i = 0; i < 256; ++i) \
            message[i] = 0; \
        for (size_t i = 0; i < 128; ++i) \
            message[i] = (i < 32 ? static_cast<uint8_t>( \
                (encoded[i / 8] ^ mask[i / 8]) >> ((i % 8) * 8)) : 0) ^ \
                (pass == 0 ? 0x36 : 0x5c); \
        size_t length = 128; \
        if (pass == 0) { \
            for (uint8_t byte : domain) \
                message[length++] = byte; \
            for (size_t i = 0; i < 32; ++i) \
                message[length++] = (hostNonce)[i]; \
            for (size_t i = 0; i < 32; ++i) \
                message[length++] = (addonNonce)[i]; \
            for (size_t i = 0; i < sizeof(identity); ++i) \
                message[length++] = reinterpret_cast<const uint8_t*>(identity)[i]; \
        } else { \
            for (size_t i = 0; i < 64; ++i) \
                message[length++] = inner[i]; \
        } \
        message[length] = 0x80; \
        for (size_t i = 0; i < 8; ++i) { \
            message[255 - i] = static_cast<uint8_t>((length * 8) >> (i * 8)); \
            state[i] = initial[i]; \
        } \
        for (size_t block = 0; block < 2; ++block) { \
            for (size_t i = 0; i < 16; ++i) { \
                uint64_t word = 0; \
                for (size_t j = 0; j < 8; ++j) \
                    word = (word << 8) | message[block * 128 + i * 8 + j]; \
                words[i] = word; \
            } \
            for (size_t i = 16; i < 80; ++i) { \
                const uint64_t a = words[i - 15], b = words[i - 2]; \
                words[i] = words[i - 16] + words[i - 7] + \
                    (PLASTICITY_HOST_ROTATE(a, 1) ^ PLASTICITY_HOST_ROTATE(a, 8) ^ (a >> 7)) + \
                    (PLASTICITY_HOST_ROTATE(b, 19) ^ PLASTICITY_HOST_ROTATE(b, 61) ^ (b >> 6)); \
            } \
            for (size_t i = 0; i < 8; ++i) \
                work[i] = state[i]; \
            for (size_t i = 0; i < 80; ++i) { \
                const uint64_t a = work[0], b = work[1], c = work[2]; \
                const uint64_t e = work[4], f = work[5], g = work[6]; \
                const uint64_t first = work[7] + constants[i] + words[i] + \
                    (PLASTICITY_HOST_ROTATE(e, 14) ^ PLASTICITY_HOST_ROTATE(e, 18) ^ PLASTICITY_HOST_ROTATE(e, 41)) + \
                    (g ^ (e & (f ^ g))); \
                const uint64_t second = \
                    (PLASTICITY_HOST_ROTATE(a, 28) ^ PLASTICITY_HOST_ROTATE(a, 34) ^ PLASTICITY_HOST_ROTATE(a, 39)) + \
                    (((a | b) & c) | (a & b)); \
                for (size_t j = 7; j > 0; --j) \
                    work[j] = work[j - 1]; \
                work[4] = work[4] + first; \
                work[0] = first + second; \
            } \
            for (size_t i = 0; i < 8; ++i) \
                state[i] = state[i] + work[i]; \
        } \
        for (size_t i = 0; i < 64; ++i) \
            inner[i] = static_cast<uint8_t>(state[i / 8] >> (56 - (i % 8) * 8)); \
    } \
    for (size_t i = 0; i < 64; ++i) { \
        (output)[i] = inner[i]; \
        inner[i] = 0; \
    } \
    for (size_t i = 0; i < 256; ++i) \
        message[i] = 0; \
    for (size_t i = 0; i < 80; ++i) \
        words[i] = 0; \
    for (size_t i = 0; i < 8; ++i) { \
        state[i] = 0; \
        work[i] = 0; \
    } \
} while (false)
