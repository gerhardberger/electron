// Copyright (c) 2026 Plastic Software LLC.

#ifndef PLASTICITY_SECURE_INCLUDE_PLASTICITY_SECURE_PLASTICITY_SECURE_H_
#define PLASTICITY_SECURE_INCLUDE_PLASTICITY_SECURE_PLASTICITY_SECURE_H_

#include "v8-forward.h"
#include "v8-local-handle.h"

namespace plasticity::secure {

v8::Local<v8::Value> RunApplication(v8::Isolate* isolate,
                                   v8::Local<v8::Context> context);

}  // namespace plasticity::secure

#endif  // PLASTICITY_SECURE_INCLUDE_PLASTICITY_SECURE_PLASTICITY_SECURE_H_
