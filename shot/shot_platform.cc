// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_platform.h"

#include <string_view>
#include <utility>

#include "ui/base/resource/resource_bundle.h"

namespace shot {

ShotPlatform::ShotPlatform() = default;
ShotPlatform::~ShotPlatform() = default;

bool ShotPlatform::HasDataResource(int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().HasDataResource(resource_id);
}

blink::WebData ShotPlatform::GetDataResource(
    int resource_id,
    ui::ResourceScaleFactor scale_factor) {
  // WebData wraps a span, and the bytes have to outlive it. ResourceBundle
  // owns the RefCountedMemory it hands back for the process's lifetime, so
  // copying the span into WebData is safe here.
  scoped_refptr<base::RefCountedMemory> bytes =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytesForScale(
          resource_id, scale_factor);
  if (!bytes) {
    return blink::WebData();
  }
  return blink::WebData(*bytes);
}

std::string ShotPlatform::GetDataResourceString(int resource_id) {
  // LoadDataResourceString, not GetRawDataResource: blink's .grd entries are
  // compress="gzip"/"brotli" and this is the accessor that decompresses. Handing
  // back the compressed bytes would give the CSS parser a binary blob and a
  // document with no user-agent styles.
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
      resource_id);
}

scoped_refptr<base::RefCountedMemory> ShotPlatform::GetDataResourceBytes(
    int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

blink::WebString ShotPlatform::DefaultLocale() {
  // The base class returns an empty WebString, and an empty one is not merely
  // uninformative: InitializePlatformLanguage() turns it into a null
  // AtomicString, and PlatformLanguage() then dereferences its null StringImpl
  // on the first document decoded.
  //
  // en-US is the honest answer for this binary rather than a placeholder: the
  // localized strings it carries are the en-US Blink and locale-settings
  // packs (see //shot:shot_strings), so this is the locale it actually has. The value is real input to rendering -- it picks the
  // language-sensitive font fallback and the default quote characters -- so it
  // has to agree with what is packed.
  return blink::WebString::FromUtf8("en-US");
}

}  // namespace shot
