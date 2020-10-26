// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shell/browser/media/media_device_id_salt.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace electron {

namespace {

const char kMediaDeviceIdSalt[] = "electron.media.device_id_salt";

// The media device ID salt is used to prevent browser fingerprinting for the
// case of loading remote content. Around is a desktop application only loading
// on our own domain. We want device IDs to remain the same in the case that
// the user loses their Preferences file.
const char kAroundMediaDeviceIdSalt[] = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";

}  // namespace

MediaDeviceIDSalt::MediaDeviceIDSalt(PrefService* pref_service) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  media_device_id_salt_.Init(kMediaDeviceIdSalt, pref_service);
  if (media_device_id_salt_.GetValue().empty()) {
    media_device_id_salt_.SetValue(kAroundMediaDeviceIdSalt);
  }
}

MediaDeviceIDSalt::~MediaDeviceIDSalt() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  media_device_id_salt_.Destroy();
}

std::string MediaDeviceIDSalt::GetSalt() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return media_device_id_salt_.GetValue();
}

// static
void MediaDeviceIDSalt::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(kMediaDeviceIdSalt, std::string());
}

// static
void MediaDeviceIDSalt::Reset(PrefService* pref_service) {
  pref_service->SetString(
      kMediaDeviceIdSalt,
      content::BrowserContext::CreateRandomMediaDeviceIDSalt());
}

}  // namespace electron
