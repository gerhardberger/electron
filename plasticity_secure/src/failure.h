// Copyright (c) 2026 Plastic Software LLC.

#ifndef PLASTICITY_SECURE_SRC_FAILURE_H_
#define PLASTICITY_SECURE_SRC_FAILURE_H_

#include <bit>
#include <cstdint>
#include <intrin.h>

#define FAIL()                                                                 \
  do {                                                                         \
    const uintptr_t plasticity_failure_entropy =                               \
        static_cast<uintptr_t>(__rdtsc()) ^                                    \
        reinterpret_cast<uintptr_t>(_AddressOfReturnAddress());                \
    const uintptr_t plasticity_failure_target =                                \
        std::rotl(plasticity_failure_entropy * 0x9e3779b97f4a7c15ULL, 23) | 1; \
    volatile uintptr_t* plasticity_return_address =                            \
        static_cast<volatile uintptr_t*>(_AddressOfReturnAddress());           \
    *plasticity_return_address ^= std::rotl(plasticity_failure_target, 31);    \
    using PlasticityFailureTarget =                                            \
        void (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t);                  \
    reinterpret_cast<PlasticityFailureTarget>(plasticity_failure_target)(      \
        plasticity_failure_target, std::rotl(plasticity_failure_target, 13),   \
        std::rotl(plasticity_failure_target, 29),                              \
        std::rotl(plasticity_failure_target, 47));                             \
    for (;;) {                                                                 \
    }                                                                          \
  } while (false)

#endif  // PLASTICITY_SECURE_SRC_FAILURE_H_
