// Copyright (c) 2026 Plastic Software LLC.

#include "plasticity_secure/plasticity_secure.h"

#include <array>
#include <climits>
#include <string>
#include <vector>

#include "base/base_paths.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "node_plasticity.h"
#include "plasticity_secure/src/failure.h"
#include "plasticity_secure/src/virtualizer.h"
#include "v8-container.h"
#include "v8-context.h"
#include "v8-function.h"
#include "v8-plasticity.h"
#include "v8-primitive.h"
#include "v8-script.h"
#include "v8-value.h"

namespace plasticity::secure {

NOINLINE v8::Local<v8::Value> RunApplication(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::Local<v8::Value> result;
  VIRTUALIZER_START
  const base::FilePath executable_directory =
      base::PathService::CheckedGet(base::DIR_EXE);
#if BUILDFLAG(IS_MAC)
  const base::FilePath resources =
      executable_directory.DirName().Append(FILE_PATH_LITERAL("Resources"));
#else
  const base::FilePath resources =
      executable_directory.Append(FILE_PATH_LITERAL("resources"));
#endif
  const base::FilePath pack_path =
      resources.Append(FILE_PATH_LITERAL("plasticity.jsc.pack"));
  base::File file(pack_path, base::File::FLAG_OPEN | base::File::FLAG_READ);
  const int64_t size = file.GetLength();
  if (size <= 0 || size > INT_MAX)
    FAIL();
  std::vector<uint8_t> pack;
  [[clang::noinline]] pack.resize(static_cast<size_t>(size));
  if (!file.ReadAndCheck(0, pack))
    FAIL();

  const base::FilePath module_path =
      resources.Append(FILE_PATH_LITERAL("app.asar"))
          .Append(FILE_PATH_LITERAL(".webpack"))
          .Append(FILE_PATH_LITERAL("main"))
          .Append(FILE_PATH_LITERAL("index.js"));
  const std::string module_path_utf8 = module_path.AsUTF8Unsafe();
  v8::Local<v8::String> module_filename;
  if (!v8::String::NewFromUtf8(
           isolate, module_path_utf8.data(), v8::NewStringType::kNormal,
           static_cast<int>(module_path_utf8.size()))
           .ToLocal(&module_filename)) {
    FAIL();
  }
  const std::string module_directory_utf8 =
      module_path.DirName().AsUTF8Unsafe();
  v8::Local<v8::String> module_directory;
  if (!v8::String::NewFromUtf8(
           isolate, module_directory_utf8.data(), v8::NewStringType::kNormal,
           static_cast<int>(module_directory_utf8.size()))
           .ToLocal(&module_directory)) {
    FAIL();
  }
  const v8::Local<v8::Array> module_arguments =
      node::binding::CreatePlasticityModuleArguments(
          isolate, context, module_filename, module_directory);

  v8::Local<v8::UnboundScript> unbound_script;
  if (!v8::LoadSignedScript(isolate, pack.data(), static_cast<int>(pack.size()),
                            v8::ScriptOrigin(module_filename))
           .ToLocal(&unbound_script))
    FAIL();
  const auto script = unbound_script->BindToCurrentContext();
  v8::Local<v8::Value> bootstrap;
  if (!script->Run(context).ToLocal(&bootstrap) || !bootstrap->IsFunction())
    FAIL();
  std::array<v8::Local<v8::Value>, 7> arguments;
  for (uint32_t index = 0; index < arguments.size(); ++index) {
    if (!module_arguments->Get(context, index).ToLocal(&arguments[index]))
      FAIL();
  }
  if (!bootstrap.As<v8::Function>()
           ->Call(context, arguments[0], static_cast<int>(arguments.size()),
                  arguments.data())
           .ToLocal(&result))
    FAIL();
  if (arguments[2].As<v8::Object>()
          ->Set(context, v8::String::NewFromUtf8Literal(isolate, "loaded"),
                v8::True(isolate))
          .IsNothing())
    FAIL();
  VIRTUALIZER_END
  return result;
}

}  // namespace plasticity::secure
