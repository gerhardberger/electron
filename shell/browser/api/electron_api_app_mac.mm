// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include <string>

#include "base/path_service.h"
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
    gfx::Point new_hot_spot = gfx::Point([cursor hotSpot]);
    if (new_hot_spot != hot_spot_) {
      hot_spot_ = new_hot_spot;
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
