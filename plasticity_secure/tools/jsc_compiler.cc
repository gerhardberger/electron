// Build-time host tool; never packaged with the application.
#ifdef UNSAFE_BUFFERS_BUILD
#pragma allow_unsafe_buffers
#endif

#include <cstdio>
#include <string>
#include <string_view>
#include <vector>

#include "cppgc/platform.h"
#include "electron/electron_version.h"
#include "node.h"
#include "v8.h"

int main(int argc, char* argv[]) {
  if (argc == 2 && std::string_view(argv[1]) == "--runtime-version") {
    std::printf("{\"electron\":\"%s\",\"v8\":\"%s\"}\n",
                ELECTRON_VERSION_STRING, v8::V8::GetVersion());
    return 0;
  }
  v8::V8::InitializeICUDefaultLocation(argv[0]);
  v8::V8::InitializeExternalStartupData(argv[0]);
  const auto initialization = node::InitializeOncePerProcess(
      std::vector<std::string>(argv, argv + argc),
      {node::ProcessInitializationFlags::kNoInitializeV8,
       node::ProcessInitializationFlags::kNoInitializeNodeV8Platform,
       node::ProcessInitializationFlags::kNoInitializeCppgc});
  for (const auto& error : initialization->errors())
    std::fprintf(stderr, "%s\n", error.c_str());
  if (initialization->early_return())
    return initialization->exit_code();

  auto platform = node::MultiIsolatePlatform::Create(4);
  v8::V8::InitializePlatform(platform.get());
  cppgc::InitializeProcess(platform->GetPageAllocator());
  v8::V8::Initialize();
  int exit_code = 1;
  {
    std::vector<std::string> errors;
    auto setup = node::CommonEnvironmentSetup::Create(
        platform.get(), &errors, initialization->args(),
        initialization->exec_args());
    for (const auto& error : errors)
      std::fprintf(stderr, "%s\n", error.c_str());
    if (setup) {
      v8::Locker locker(setup->isolate());
      v8::Isolate::Scope isolate_scope(setup->isolate());
      v8::HandleScope handle_scope(setup->isolate());
      v8::Context::Scope context_scope(setup->context());
      if (!node::LoadEnvironment(setup->env(), node::StartExecutionCallback{})
               .IsEmpty())
        exit_code = node::SpinEventLoop(setup->env()).FromMaybe(1);
      node::Stop(setup->env());
    }
  }
  v8::V8::Dispose();
  v8::V8::DisposePlatform();
  node::TearDownOncePerProcess();
  return exit_code;
}
