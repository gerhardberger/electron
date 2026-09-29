// Copyright (c) 2026 Plastic Software LLC.

#ifndef PLASTICITY_SECURE_SRC_ASSET_PACK_H_
#define PLASTICITY_SECURE_SRC_ASSET_PACK_H_

#include <cstddef>
#include <cstdint>
#include <memory>

namespace plasticity::secure {

inline constexpr size_t kAssetHeaderSize = 140;
inline constexpr size_t kAssetSignatureSize = 64;
inline constexpr size_t kAssetAuthenticatedHeaderSize = 64;
inline constexpr size_t kAddonSignatureSize = 64;

// Set only after authenticating a JSC pack, before executing its script.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wexit-time-destructors"
// Each thread owns its signature until replacement or thread exit.
inline thread_local std::unique_ptr<uint8_t[]> addon_signature;
#pragma clang diagnostic pop

inline constexpr size_t kSourceLengthOffset = 32;
inline constexpr size_t kRuntimeIdOffset = 36;
inline constexpr size_t kJscNonceOffset = 64;
inline constexpr size_t kJscTagOffset = 80;
inline constexpr size_t kWrapNonceOffset = 96;
inline constexpr size_t kWrappedKeyOffset = 108;
inline constexpr size_t kWrappedKeyTagOffset = 124;

}  // namespace plasticity::secure

#endif  // PLASTICITY_SECURE_SRC_ASSET_PACK_H_
