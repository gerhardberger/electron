// Copyright (c) 2019 GitHub, Inc.
// Use of this source code is governed by the MIT license that can be
// found in the LICENSE file.

#include "base/path_service.h"
#include "shell/browser/api/atom_api_app.h"
#include "shell/browser/atom_paths.h"
#include "shell/common/native_mate_converters/file_path_converter.h"

#import <AudioToolbox/AudioServices.h>
#import <Cocoa/Cocoa.h>
#import <CoreAudio/CoreAudio.h>

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
  }
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

void App::SetupAudioEventPassing() {
  AudioObjectPropertyAddress virtualOutputMasterVolumePropertyAddress{
      .mScope = kAudioDevicePropertyScopeOutput,
      .mElement = kAudioObjectPropertyElementMaster,
      .mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume};

  AudioObjectAddPropertyListener(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultOutputDevice),
      &virtualOutputMasterVolumePropertyAddress, onOutputVolumeChange,
      (void*)this);

  AudioObjectPropertyAddress virtualInputMasterVolumePropertyAddress{
      .mScope = kAudioDevicePropertyScopeInput,
      .mElement = kAudioObjectPropertyElementMaster,
      .mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume};

  AudioObjectAddPropertyListener(
      obtainDefaultAudioDevice(kAudioHardwarePropertyDefaultInputDevice),
      &virtualInputMasterVolumePropertyAddress, onInputVolumeChange,
      (void*)this);
}

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
                         base::Optional<base::FilePath> custom_path) {
  if (custom_path.has_value()) {
    if (!custom_path->IsAbsolute()) {
      thrower.ThrowError("Path must be absolute");
      return;
    }
    base::PathService::Override(DIR_APP_LOGS, custom_path.value());
  } else {
    NSString* bundle_name =
        [[[NSBundle mainBundle] infoDictionary] objectForKey:@"CFBundleName"];
    NSString* logs_path =
        [NSString stringWithFormat:@"Library/Logs/%@", bundle_name];
    NSString* library_path =
        [NSHomeDirectory() stringByAppendingPathComponent:logs_path];
    base::PathService::Override(DIR_APP_LOGS,
                                base::FilePath([library_path UTF8String]));
  }
}

}  // namespace api

}  // namespace electron
