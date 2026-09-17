// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/fonts/linux/font_unique_name_lookup_linux.h"

#include "third_party/blink/public/platform/linux/web_sandbox_support.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/platform/fonts/skia/sktypeface_factory.h"
#include "ui/gfx/font_fallback_linux.h"

namespace blink {

FontUniqueNameLookupLinux::~FontUniqueNameLookupLinux() = default;

sk_sp<SkTypeface> FontUniqueNameLookupLinux::MatchUniqueName(
    const String& font_unique_name) {
  WebSandboxSupport* sandbox_support = Platform::Current()->GetSandboxSupport();
  if (!sandbox_support) {
    // A non-browser embedder is allowed to have no browser-side font service.
    // Treat local() as an ordinary miss so the next src: url() candidate can
    // be tried instead of logging an error for a supported fallback path.
    return nullptr;
  }

  gfx::FallbackFontData uniquely_matched_font;
  if (!sandbox_support->MatchFontByPostscriptNameOrFullFontName(
          font_unique_name.Utf8(Utf8ConversionMode::kStrict).c_str(),
          &uniquely_matched_font)) {
    return nullptr;
  }

  // Chrome's sandbox service normally returns an opaque fontconfig id. Shotium
  // resolves local() in-process and therefore already has the actual file path;
  // prefer it when present instead of manufacturing a browser-process id.
  if (!uniquely_matched_font.filepath.empty()) {
    return SkTypeface_Factory::FromFilenameAndTtcIndex(
        uniquely_matched_font.filepath.AsUTF8Unsafe(),
        uniquely_matched_font.ttc_index);
  }

  return SkTypeface_Factory::FromFontConfigInterfaceIdAndTtcIndex(
      uniquely_matched_font.fontconfig_interface_id,
      uniquely_matched_font.ttc_index);
}

}  // namespace blink
