// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "base/time/time.h"
#include "shell/browser/api/electron_api_app.h"
#include "shell/common/api/electron_api_native_image.h"
#include "shell/common/electron_paths.h"
#include "shell/common/node_includes.h"
#include "shell/common/process_util.h"

#import <AudioToolbox/AudioServices.h>
#import <Cocoa/Cocoa.h>
#import <CoreAudio/CoreAudio.h>
#import <sys/sysctl.h>

namespace electron {

namespace api {

AudioDeviceID obtainDefaultAudioDevice(AudioObjectPropertySelector selector) {
  AudioDeviceID deviceID = kAudioObjectUnknown;
  AudioObjectPropertyAddress address{
      .mSelector = selector,
      .mScope = kAudioObjectPropertyScopeGlobal,
      .mElement = kAudioObjectPropertyElementMaster};

  if (!AudioObjectHasProperty(kAudioObjectSystemObject, &address)) {
    return deviceID;
  }

  UInt32 size = sizeof(deviceID);
  OSStatus err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address,
                                            0, NULL, &size, &deviceID);
  if (err != noErr) {
    return kAudioObjectUnknown;
  }

  return deviceID;
}

/**
 * Retrieves an audio device name, also called 'label' in Chromium terms, based
 * on the provided device ID.
 */
bool getAudioDeviceNameFromDeviceId(
    const AudioObjectPropertyScope& scope /*input*/,
    const AudioObjectID& deviceId /*input*/,
    std::u16string& deviceName /*output*/,
    std::string& error /*output*/) {
  // First we try to take the name from the device "source"
  // and if that comes back blank then we take the normal friendly name. See
  // media/audio/mac/core_audio_util_mac.cc :: GetDeviceLabel()
  OSStatus status = kAudioHardwareNoError;
  CFStringRef deviceNameCfString = nullptr;
  UInt32 propertySize = 0;

  // Try to use source for getting the name
  AudioObjectPropertyAddress deviceSourceAddress = {
      kAudioDevicePropertyDataSource, scope, kAudioObjectPropertyElementMaster};
  UInt32 sourceId = 0;
  status = AudioObjectGetPropertyData(deviceId, &deviceSourceAddress, 0, NULL,
                                      &propertySize, &sourceId);
  if (status == kAudioHardwareNoError && sourceId != 0) {
    AudioValueTranslation translation;
    translation.mInputData = &sourceId;
    translation.mInputDataSize = sizeof(sourceId);
    translation.mOutputData = &deviceNameCfString;
    translation.mOutputDataSize = sizeof(deviceNameCfString);

    propertySize = sizeof(translation);
    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyDataSourceNameForIDCFString, scope,
        kAudioObjectPropertyElementMaster};
    status = AudioObjectGetPropertyData(
        deviceId, &propertyAddress, 0 /* inQualifierDataSize */,
        nullptr /* inQualifierData */, &propertySize, &translation);
    if (status != kAudioHardwareNoError) {
      // Don't error out, try normal friendly name below.
      deviceNameCfString = nullptr;
    }
  }

  // If still null, try normal friendly name
  if (deviceNameCfString == nullptr) {
    AudioObjectPropertyAddress propertyAddress = {
        kAudioObjectPropertyName, kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster};
    propertySize = sizeof(deviceNameCfString);
    status = AudioObjectGetPropertyData(deviceId, &propertyAddress, 0, NULL,
                                        &propertySize, &deviceNameCfString);
    if (status != kAudioHardwareNoError) {
      error = (std::string("Error getting device name from ID | OSStatus:") +
               std::to_string(status));
      return false;
    }
  }

  deviceName = base::SysCFStringRefToUTF16(deviceNameCfString);
  CFRelease(deviceNameCfString);
  return true;
}

/**
 * Sets the OS default device, either input or output depending on the provided
 * scope.
 * error: if error is encountered this string contains the error message
 * requestedDeviceName: Device name to try to set.
 * scope: kAudioDevicePropertyScopeInput | kAudioDevicePropertyScopeOutput
 * returns: false if error
 */
bool setSystemDefaultAudioDeviceByName(
    const AudioObjectPropertyScope& scope /*input*/,
    const std::u16string& requestedDeviceName /*input*/,
    std::string& error /*output*/) {
  OSStatus status = kAudioHardwareNoError;
  UInt32 propertySize = 0;

  AudioObjectPropertyAddress globalScopeAddress = {
      0, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster};
  AudioObjectPropertyAddress specificScopeAddress = {
      scope, kAudioDevicePropertyScopeOutput,
      kAudioObjectPropertyElementMaster};

  // Get the size of the array of aduio devices.
  globalScopeAddress.mSelector = kAudioHardwarePropertyDevices;
  status = AudioObjectGetPropertyDataSize(
      kAudioObjectSystemObject, &globalScopeAddress, 0, nullptr, &propertySize);
  if (status != kAudioHardwareNoError) {
    error =
        (std::string("Error getting size of audio device list | OSStatus:") +
         std::to_string(status));
    return false;
  }

  if (propertySize == 0U) {
    error = "No audio devices found";
    return false;
  }

  size_t numberOfDevices = propertySize / sizeof(AudioObjectID);

  // Get the array of device ids for all the devices, which includes both
  // input devices and output devices.
  std::vector<AudioObjectID> devices(numberOfDevices);
  globalScopeAddress.mSelector = kAudioHardwarePropertyDevices;
  status =
      AudioObjectGetPropertyData(kAudioObjectSystemObject, &globalScopeAddress,
                                 0, nullptr, &propertySize, devices.data());

  for (size_t i = 0U; i < numberOfDevices; i++) {
    // Get the device name.
    std::u16string deviceName;
    if (!getAudioDeviceNameFromDeviceId(scope, devices[i], deviceName, error)) {
      return false;
    }

    if (deviceName == requestedDeviceName) {
      specificScopeAddress.mSelector =
          scope == kAudioDevicePropertyScopeInput
              ? kAudioHardwarePropertyDefaultInputDevice
              : kAudioHardwarePropertyDefaultOutputDevice;
      status = AudioObjectSetPropertyData(kAudioObjectSystemObject,
                                          &specificScopeAddress, 0, NULL,
                                          sizeof(AudioObjectID), &devices[i]);
      if (status != kAudioHardwareNoError) {
        error = (std::string("Error setting device as default | OSStatus:") +
                 std::to_string(status));
        return false;
      }

      return true;
    }
  }

  error = std::string("No matching device found");
  return false;
}

/**
 * Gets the OS default device, either input or output depending on the provided
 * scope. This isn't quite the same as the friendly name and the implementation
 * is taken from Chromium's media/audio/mac/core_audio_util_mac.cc
 * error: if error is encountered this string contains the error message
 * deviceName: return value scope: kAudioDevicePropertyScopeInput |
 * kAudioDevicePropertyScopeOutput returns: false if error
 */
bool getSystemDefaultAudioDeviceName(
    const AudioObjectPropertyScope& scope /*input*/,
    std::u16string& deviceName /*output*/,
    std::string& error /*output*/) {
  OSStatus status = kAudioHardwareNoError;
  AudioObjectID deviceId = kAudioObjectUnknown;
  UInt32 propertySize = 0;

  // Get the ID of the default device.
  AudioObjectPropertyAddress propertyAddress = {
      (scope == kAudioDevicePropertyScopeInput
           ? kAudioHardwarePropertyDefaultInputDevice
           : kAudioHardwarePropertyDefaultOutputDevice),
      kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMaster};
  propertySize = sizeof(deviceId);
  status =
      AudioObjectGetPropertyData(kAudioObjectSystemObject, &propertyAddress, 0,
                                 NULL, &propertySize, &deviceId);
  if (status != kAudioHardwareNoError) {
    error = (std::string("Error getting default device ID | OSStatus:") +
             std::to_string(status));
    return false;
  }

  // Get the device name.
  return getAudioDeviceNameFromDeviceId(scope, deviceId, deviceName, error);
}

void setSystemMuted(bool muted,
                    AudioDeviceID defaultDeviceID,
                    AudioObjectPropertyScope scope) {
  if (defaultDeviceID == kAudioObjectUnknown) {
    return;
  }

  AudioObjectPropertyAddress address{
      .mSelector = kAudioDevicePropertyMute,
      .mScope = scope,
      .mElement = kAudioObjectPropertyElementMaster};

  UInt32 newValue = muted ? 1 : 0;
  OSStatus err = AudioObjectSetPropertyData(defaultDeviceID, &address, 0, NULL,
                                            sizeof(newValue), &newValue);
  if (err != noErr) {
    NSLog(@"Could not set audio muted");
  }
}

bool isSystemMuted(AudioDeviceID defaultDeviceID,
                   AudioObjectPropertyScope scope) {
  if (defaultDeviceID == kAudioObjectUnknown) {
    return false;
  }

  AudioObjectPropertyAddress address{
      .mSelector = kAudioDevicePropertyMute,
      .mScope = scope,
      .mElement = kAudioObjectPropertyElementMaster};

  UInt32 muted = 0;
  UInt32 mutedSize = sizeof(muted);
  OSStatus err = AudioObjectGetPropertyData(defaultDeviceID, &address, 0, NULL,
                                            &mutedSize, &muted);
  if (err != noErr) {
    return false;
  }

  return muted != 0;
}

float getSystemVolume(AudioDeviceID defaultDeviceID,
                      AudioObjectPropertyScope scope) {
  if (defaultDeviceID == kAudioObjectUnknown) {
    return 0.0;
  }

  AudioObjectPropertyAddress address{
      .mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume,
      .mScope = scope,
      .mElement = kAudioObjectPropertyElementMaster};

  float volume = 0;
  UInt32 size = sizeof(volume);
  OSStatus err = AudioObjectGetPropertyData(defaultDeviceID, &address, 0, NULL,
                                            &size, &volume);
  if (err != noErr) {
    return 0.0;
  }

  return volume > 1.0 ? 1.0 : (volume < 0.0 ? 0.0 : volume);
}

void setSystemVolume(float volume,
                     AudioDeviceID defaultDeviceID,
                     AudioObjectPropertyScope scope) {
  if (defaultDeviceID == kAudioObjectUnknown) {
    return;
  }

  AudioObjectPropertyAddress address{
      .mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume,
      .mScope = scope,
      .mElement = kAudioObjectPropertyElementMaster};

  float newValue = volume > 1.0 ? 1.0 : (volume < 0.0 ? 0.0 : volume);
  OSStatus err = AudioObjectSetPropertyData(defaultDeviceID, &address, 0, NULL,
                                            sizeof(newValue), &newValue);
  if (err != noErr) {
    NSLog(@"Could not set audio volume");
    return;
  }

  if (newValue == 0.0) {
    setSystemMuted(true, defaultDeviceID, scope);
  }

  if (newValue > 0.0 && isSystemMuted(defaultDeviceID, scope)) {
    setSystemMuted(false, defaultDeviceID, scope);
  }
}

static OSStatus onOutputVolumeChange(
    AudioObjectID inObjectID,
    UInt32 inNumberAddresses,
    const AudioObjectPropertyAddress* inAddresses,
    void* inClientData) {
  dispatch_async(dispatch_get_main_queue(), ^{
    auto* self = static_cast<App*>(inClientData);
    if (self) {
      self->Emit("system-output-volume-changed");
    }
  });
  return noErr;
}

static OSStatus onInputVolumeChange(
    AudioObjectID inObjectID,
    UInt32 inNumberAddresses,
    const AudioObjectPropertyAddress* inAddresses,
    void* inClientData) {
  dispatch_async(dispatch_get_main_queue(), ^{
    auto* self = static_cast<App*>(inClientData);
    if (self) {
      self->Emit("system-input-volume-changed");
    }
  });
  return noErr;
}

static OSStatus onOutputMuteChange(
    AudioObjectID inObjectID,
    UInt32 inNumberAddresses,
    const AudioObjectPropertyAddress* inAddresses,
    void* inClientData) {
  dispatch_async(dispatch_get_main_queue(), ^{
    auto* self = static_cast<App*>(inClientData);
    if (self) {
      self->Emit("system-output-mute-changed");
    }
  });
  return noErr;
}

static OSStatus onInputMuteChange(AudioObjectID inObjectID,
                                  UInt32 inNumberAddresses,
                                  const AudioObjectPropertyAddress* inAddresses,
                                  void* inClientData) {
  dispatch_async(dispatch_get_main_queue(), ^{
    auto* self = static_cast<App*>(inClientData);
    if (self) {
      self->Emit("system-input-mute-changed");
    }
  });
  return noErr;
}

void App::SetupAudioEventPassing() {
  AudioObjectPropertyAddress virtualOutputMasterVolumePropertyAddress{
      .mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume,
      .mScope = kAudioDevicePropertyScopeOutput,
      .mElement = kAudioObjectPropertyElementMaster};

  AudioObjectAddPropertyListener(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultOutputDevice),
      &virtualOutputMasterVolumePropertyAddress, onOutputVolumeChange,
      (void*)this);

  AudioObjectPropertyAddress virtualInputMasterVolumePropertyAddress{
      .mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume,
      .mScope = kAudioDevicePropertyScopeInput,
      .mElement = kAudioObjectPropertyElementMaster};

  AudioObjectAddPropertyListener(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultInputDevice),
      &virtualInputMasterVolumePropertyAddress, onInputVolumeChange,
      (void*)this);

  AudioObjectPropertyAddress virtualOutputMasterMutePropertyAddress{
      .mSelector = kAudioDevicePropertyMute,
      .mScope = kAudioDevicePropertyScopeOutput,
      .mElement = kAudioObjectPropertyElementMaster};

  AudioObjectAddPropertyListener(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultOutputDevice),
      &virtualOutputMasterMutePropertyAddress, onOutputMuteChange, (void*)this);

  AudioObjectPropertyAddress virtualInputMasterMutePropertyAddress{
      .mSelector = kAudioDevicePropertyMute,
      .mScope = kAudioDevicePropertyScopeInput,
      .mElement = kAudioObjectPropertyElementMaster};

  AudioObjectAddPropertyListener(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultInputDevice),
      &virtualInputMasterMutePropertyAddress, onInputMuteChange, (void*)this);
}

void App::TeardownAudioEventPassing() {}

float App::GetSystemOutputVolume() {
  return getSystemVolume(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultOutputDevice),
      kAudioDevicePropertyScopeOutput);
}

float App::GetSystemInputVolume() {
  return getSystemVolume(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultInputDevice),
      kAudioDevicePropertyScopeInput);
}

void App::SetSystemOutputVolume(float volume) {
  return setSystemVolume(
      volume,
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultOutputDevice),
      kAudioDevicePropertyScopeOutput);
}

void App::SetSystemInputVolume(float volume) {
  return setSystemVolume(
      volume,
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultInputDevice),
      kAudioDevicePropertyScopeInput);
}

bool App::IsSystemOutputMuted() {
  return isSystemMuted(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultOutputDevice),
      kAudioDevicePropertyScopeOutput);
}

bool App::IsSystemInputMuted() {
  return isSystemMuted(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultInputDevice),
      kAudioDevicePropertyScopeInput);
}

void App::SetSystemOutputMuted(bool muted) {
  return setSystemMuted(
      muted,
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultOutputDevice),
      kAudioDevicePropertyScopeOutput);
}

void App::SetSystemInputMuted(bool muted) {
  return setSystemMuted(
      muted, obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultInputDevice),
      kAudioDevicePropertyScopeInput);
}

void App::SetSystemOutputDevice(const std::u16string& device_name) {
  std::string error;

  if (!setSystemDefaultAudioDeviceByName(kAudioDevicePropertyScopeOutput,
                                         device_name, error)) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::Locker locker(isolate);
    v8::HandleScope scope(isolate);
    gin_helper::ErrorThrower(isolate).ThrowError(error);
  }

  return;
}

std::u16string App::GetSystemOutputDevice() {
  std::string error;
  std::u16string device_name;

  if (!getSystemDefaultAudioDeviceName(kAudioDevicePropertyScopeOutput,
                                       device_name, error)) {
    v8::Isolate* isolate = JavascriptEnvironment::GetIsolate();
    v8::Locker locker(isolate);
    v8::HandleScope scope(isolate);
    gin_helper::ErrorThrower(isolate).ThrowError(error);
  }

  return device_name;
}

void App::SetAppLogsPath(gin_helper::ErrorThrower thrower,
                         absl::optional<base::FilePath> custom_path) {
  if (custom_path.has_value()) {
    if (!custom_path->IsAbsolute()) {
      thrower.ThrowError("Path must be absolute");
      return;
    }
    {
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      base::PathService::Override(DIR_APP_LOGS, custom_path.value());
    }
  } else {
    NSString* bundle_name =
        [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
    NSString* logs_path =
        [NSString stringWithFormat:@"Library/Logs/%@", bundle_name];
    NSString* library_path =
        [NSHomeDirectory() stringByAppendingPathComponent:logs_path];
    {
      base::ThreadRestrictions::ScopedAllowIO allow_io;
      base::PathService::Override(DIR_APP_LOGS,
                                  base::FilePath([library_path UTF8String]));
    }
  }
}

void App::SetActivationPolicy(gin_helper::ErrorThrower thrower,
                              const std::string& policy) {
  NSApplicationActivationPolicy activation_policy;
  if (policy == "accessory") {
    activation_policy = NSApplicationActivationPolicyAccessory;
  } else if (policy == "prohibited") {
    activation_policy = NSApplicationActivationPolicyProhibited;
  } else if (policy == "regular") {
    activation_policy = NSApplicationActivationPolicyRegular;
  } else {
    thrower.ThrowError("Invalid activation policy: must be one of 'regular', "
                       "'accessory', or 'prohibited'");
    return;
  }

  [NSApp setActivationPolicy:activation_policy];
}

bool App::IsRunningUnderRosettaTranslation() const {
  node::Environment* env =
      node::Environment::GetCurrent(JavascriptEnvironment::GetIsolate());

  EmitWarning(env,
              "The app.runningUnderRosettaTranslation API is deprecated, use "
              "app.runningUnderARM64Translation instead.",
              "electron");
  return IsRunningUnderARM64Translation();
}

bool App::IsRunningUnderARM64Translation() const {
  int proc_translated = 0;
  size_t size = sizeof(proc_translated);
  if (sysctlbyname("sysctl.proc_translated", &proc_translated, &size, NULL,
                   0) == -1) {
    return false;
  }
  return proc_translated == 1;
}

void App::EmitCursorChange() {
  NSCursor* cursor = [NSCursor currentSystemCursor];
  if (cursor) {
    NSImage* new_cursor_image = [cursor image];
    if (![[new_cursor_image TIFFRepresentation]
            isEqual:[cursor_image_.AsNSImage() TIFFRepresentation]]) {
      cursor_image_ = gfx::Image(new_cursor_image);
      Emit("system-cursor-changed");
    }
  }
}

gin_helper::Dictionary App::GetSystemCursor(v8::Isolate* isolate) {
  gin_helper::Dictionary result = gin::Dictionary::CreateEmpty(isolate);

  NSCursor* cursor = [NSCursor currentSystemCursor];
  if (cursor) {
    gfx::Point hot_spot = gfx::Point([cursor hotSpot]);
    gin_helper::Dictionary hot_spot_dict =
        gin::Dictionary::CreateEmpty(isolate);
    hot_spot_dict.SetHidden("simple", true);
    hot_spot_dict.Set("x", hot_spot.x());
    hot_spot_dict.Set("y", hot_spot.y());
    result.Set("hotSpot", hot_spot_dict.GetHandle());

    gfx::Image image = gfx::Image([cursor image]);
    result.Set("image", NativeImage::Create(isolate, image));
  }

  return result;
}

}  // namespace api

}  // namespace electron
