// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_platform.h"

#include <memory>
#include <optional>
#include <string_view>
#include <utility>

#include "build/build_config.h"
#include "ui/base/resource/resource_bundle.h"

#if BUILDFLAG(IS_LINUX)
#include <fontconfig/fontconfig.h>

#include "shot/shot_capture_context.h"
#include "third_party/blink/public/platform/linux/web_sandbox_support.h"
#include "third_party/blink/public/platform/web_font_render_style.h"
#include "ui/gfx/font.h"
#include "ui/gfx/font_fallback_linux.h"
#include "ui/gfx/font_render_params.h"
#include "ui/gfx/linux/fontconfig_util.h"
#endif

namespace shot {

#if BUILDFLAG(IS_LINUX)
namespace {

std::optional<gfx::FallbackFontData> FindLocalFontByName(
    const char* font_unique_name) {
  if (!font_unique_name || !*font_unique_name) {
    return std::nullopt;
  }

  for (const char* field : {FC_POSTSCRIPT_NAME, FC_FULLNAME}) {
    gfx::ScopedFcPattern pattern(FcPatternCreate());
    if (!pattern) {
      return std::nullopt;
    }
    FcPatternAddString(
        pattern.get(), field,
        reinterpret_cast<const FcChar8*>(font_unique_name));
    FcPatternAddBool(pattern.get(), FC_SCALABLE, FcTrue);

    std::unique_ptr<FcObjectSet, decltype(&FcObjectSetDestroy)> objects(
        FcObjectSetCreate(), &FcObjectSetDestroy);
    if (!objects) {
      return std::nullopt;
    }
    FcObjectSetAdd(objects.get(), FC_FILE);
    FcObjectSetAdd(objects.get(), FC_INDEX);

    std::unique_ptr<FcFontSet, decltype(&FcFontSetDestroy)> fonts(
        FcFontList(gfx::GetGlobalFontConfig(), pattern.get(), objects.get()),
        &FcFontSetDestroy);
    if (!fonts || fonts->nfont == 0) {
      continue;
    }

    FcPattern* match = fonts->fonts[0];
    base::FilePath path = gfx::GetFontPath(match);
    if (path.empty()) {
      continue;
    }
    const int ttc_index = gfx::GetFontTtcIndex(match);
    if (ttc_index < 0) {
      continue;
    }

    gfx::FallbackFontData result;
    result.filepath = std::move(path);
    result.ttc_index = ttc_index;
    return result;
  }

  return std::nullopt;
}

class ShotSandboxSupport final : public blink::WebSandboxSupport {
 public:
  bool GetFallbackFontForCharacter(
      blink::WebUChar32 character,
      const char* preferred_locale,
      gfx::FallbackFontData* fallback_font) override {
    return gfx::GetFallbackFontForChar(
        character, preferred_locale ? preferred_locale : std::string(),
        fallback_font);
  }

  bool MatchFontByPostscriptNameOrFullFontName(
      const char* font_unique_name,
      gfx::FallbackFontData* fallback_font) override {
    CaptureContext* capture = CaptureContext::Current();
    if (!capture || !capture->allow_file_access()) {
      return false;
    }

    std::optional<gfx::FallbackFontData> match =
        FindLocalFontByName(font_unique_name);
    if (!match) {
      return false;
    }
    *fallback_font = std::move(*match);
    return true;
  }

  void GetWebFontRenderStyleForStrike(
      const char* family,
      int size,
      bool is_bold,
      bool is_italic,
      float device_scale_factor,
      blink::WebFontRenderStyle* out) override {
    *out = blink::WebFontRenderStyle();
    if (size < 0) {
      return;
    }

    gfx::FontRenderParamsQuery query;
    if (family && *family) {
      query.families.emplace_back(family);
    }
    query.pixel_size = size;
    query.style = is_italic ? gfx::Font::ITALIC : gfx::Font::NORMAL;
    query.weight =
        is_bold ? gfx::Font::Weight::BOLD : gfx::Font::Weight::NORMAL;
    query.device_scale_factor = device_scale_factor;
    const gfx::FontRenderParams params =
        gfx::GetFontRenderParams(query, nullptr);

    out->use_bitmaps = params.use_bitmaps;
    out->use_auto_hint = params.autohinter;
    out->use_hinting = params.hinting != gfx::FontRenderParams::HINTING_NONE;
    out->hint_style = static_cast<char>(params.hinting);
    out->use_anti_alias = params.antialiasing;
    out->use_subpixel_rendering =
        params.subpixel_rendering !=
        gfx::FontRenderParams::SUBPIXEL_RENDERING_NONE;
    out->use_subpixel_positioning = params.subpixel_positioning;
  }
};

}  // namespace
#endif

ShotPlatform::ShotPlatform() {
#if BUILDFLAG(IS_LINUX)
  sandbox_support_ = std::make_unique<ShotSandboxSupport>();
#endif
}

ShotPlatform::~ShotPlatform() = default;

blink::WebSandboxSupport* ShotPlatform::GetSandboxSupport() {
  return sandbox_support_.get();
}

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
