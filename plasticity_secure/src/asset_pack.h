// Copyright (c) 2026 Plastic Software LLC.

#ifndef PLASTICITY_SECURE_SRC_ASSET_PACK_H_
#define PLASTICITY_SECURE_SRC_ASSET_PACK_H_

#include <cstddef>

namespace plasticity::secure {

inline constexpr size_t kAssetHeaderSize = 140;
inline constexpr size_t kAssetSignatureSize = 64;
inline constexpr size_t kAssetAuthenticatedHeaderSize = 64;

inline constexpr size_t kSourceLengthOffset = 32;
inline constexpr size_t kRuntimeIdOffset = 36;
inline constexpr size_t kJscNonceOffset = 64;
inline constexpr size_t kJscTagOffset = 80;
inline constexpr size_t kWrapNonceOffset = 96;
inline constexpr size_t kWrappedKeyOffset = 108;
inline constexpr size_t kWrappedKeyTagOffset = 124;

}  // namespace plasticity::secure

#endif  // PLASTICITY_SECURE_SRC_ASSET_PACK_H_
