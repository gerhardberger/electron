// Copyright (c) 2026 Plastic Software LLC.

#ifndef PLASTICITY_SECURE_SRC_VIRTUALIZER_H_
#define PLASTICITY_SECURE_SRC_VIRTUALIZER_H_

#define CV_X64_INSERT_VIA_INLINE
#include "../third_party/oreans/code_virtualizer/include/VirtualizerSDK.h"
#if defined(__x86_64__) || defined(_M_X64)
#undef VIRTUALIZER_START
#undef VIRTUALIZER_END
#define VIRTUALIZER_START VIRTUALIZER_TIGER_WHITE_START
#define VIRTUALIZER_END VIRTUALIZER_TIGER_WHITE_END
#endif

#if defined(_WIN32)
// Keep protected string references close enough for Oreans' string detection.
#pragma section(".text$ps", read, execute)
#define VIRTUALIZER_CODE __attribute__((noinline, section(".text$ps")))
#define VIRTUALIZER_STRING __declspec(allocate(".text$ps"))
#else
#define VIRTUALIZER_CODE
#define VIRTUALIZER_STRING
#endif

// ARM64 loaders need stable block placement for Oreans.
#if defined(__aarch64__)
#define VIRTUALIZER_LOADER_CODE __attribute__((noinline, optnone))
#else
#define VIRTUALIZER_LOADER_CODE VIRTUALIZER_CODE __attribute__((noinline))
#endif

#endif  // PLASTICITY_SECURE_SRC_VIRTUALIZER_H_
