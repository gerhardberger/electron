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

#endif  // PLASTICITY_SECURE_SRC_VIRTUALIZER_H_
