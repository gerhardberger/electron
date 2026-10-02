// Reserved protection space, matching Plasticity's native-addon Stealth.cc.
// This function is retained by the linker but must never execute.
#include <cstdint>

#include "../third_party/oreans/code_virtualizer/include/StealthCodeArea.h"

#define STEALTH_AREA_4 \
  STEALTH_AREA_CHUNK   \
  STEALTH_AREA_CHUNK   \
  STEALTH_AREA_CHUNK   \
  STEALTH_AREA_CHUNK

#define STEALTH_AREA_16 \
  STEALTH_AREA_4        \
  STEALTH_AREA_4        \
  STEALTH_AREA_4        \
  STEALTH_AREA_4

#define STEALTH_AREA_64 \
  STEALTH_AREA_16       \
  STEALTH_AREA_16       \
  STEALTH_AREA_16       \
  STEALTH_AREA_16

#define STEALTH_AREA_256 \
  STEALTH_AREA_64        \
  STEALTH_AREA_64        \
  STEALTH_AREA_64        \
  STEALTH_AREA_64

#if defined(_WIN32)
#include <windows.h>

#pragma optimize("", off)
#pragma comment(linker, "/include:MyFunctionStealthCodeArea")

__declspec(noinline) void CVSX(unsigned int i) {
  Sleep(i);
  Sleep(i);
}
#else
STEALTH_AUX_FUNCTION
#endif

#ifdef __APPLE__
struct PlasticityStealthRegisters {
  std::uint32_t start[4];
  std::uint32_t stealth[0x10000];
  std::uint32_t end[4];
};

volatile PlasticityStealthRegisters plasticity_stealth_registers = {
    {0xA5A6A7A8, 0xA8A7A6A5, 0xB5A5B6A6, 0xB7A7A5A5},
    {0},
    {0xB8B7B6B5, 0xA6B6C6D6, 0xA7A6A4A4, 0xA4A5B1B2}};
#endif

#if defined(_WIN32)
extern "C" __declspec(noinline) void MyFunctionStealthCodeArea() {
#else
__attribute__((used, retain, noinline)) void MyFunctionStealthCodeArea() {
#endif
#ifdef __APPLE__
  plasticity_stealth_registers.stealth[0] = 1;
#endif
  STEALTH_AREA_START
  STEALTH_AREA_256
#if defined(__APPLE__) && defined(__aarch64__)
  STEALTH_AREA_256
  STEALTH_AREA_64
  STEALTH_AREA_64
  STEALTH_AREA_64
  STEALTH_AREA_16
  STEALTH_AREA_16
  STEALTH_AREA_16
#else
  STEALTH_AREA_256
  STEALTH_AREA_256
  STEALTH_AREA_64
  STEALTH_AREA_64
#if !defined(__APPLE__)
  STEALTH_AREA_64
#endif
#endif
  STEALTH_AREA_END
}
