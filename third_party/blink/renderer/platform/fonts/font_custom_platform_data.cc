/*
 * Copyright (C) 2007 Apple Computer, Inc.
 * Copyright (c) 2007, 2008, 2009, Google Inc. All rights reserved.
 * Copyright (C) 2010 Company 100, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/platform/fonts/font_custom_platform_data.h"

#include <array>
#include <optional>

#include "base/containers/heap_array.h"
#include "base/synchronization/lock.h"
#include "base/thread_annotations.h"
#include "crypto/hash.h"
#include "base/logging.h"
#include "build/build_config.h"
#include "third_party/blink/renderer/platform/fonts/font_cache.h"
#include "third_party/blink/renderer/platform/fonts/font_platform_data.h"
#include "third_party/blink/renderer/platform/fonts/font_selection_types.h"
#include "third_party/blink/renderer/platform/fonts/opentype/font_format_check.h"
#include "third_party/blink/renderer/platform/fonts/opentype/font_settings.h"
#include "third_party/blink/renderer/platform/fonts/opentype/variable_axes_names.h"
#include "third_party/blink/renderer/platform/fonts/palette_interpolation.h"
#include "third_party/blink/renderer/platform/fonts/web_font_decoder.h"
#include "third_party/blink/renderer/platform/fonts/web_font_typeface_factory.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"
#include "third_party/blink/renderer/platform/wtf/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/wtf_size_t.h"
#include "third_party/skia/include/core/SkTypeface.h"

namespace {

constexpr SkFourByteTag kOpszTag = SkSetFourByteTag('o', 'p', 's', 'z');
constexpr SkFourByteTag kSlntTag = SkSetFourByteTag('s', 'l', 'n', 't');
constexpr SkFourByteTag kWdthTag = SkSetFourByteTag('w', 'd', 't', 'h');
constexpr SkFourByteTag kWghtTag = SkSetFourByteTag('w', 'g', 'h', 't');

std::optional<SkFontParameters::Variation::Axis>
RetrieveVariationDesignParametersByTag(sk_sp<SkTypeface> base_typeface,
                                       SkFourByteTag tag) {
  int axes_count = base_typeface->getVariationDesignParameters({});
  if (axes_count <= 0)
    return std::nullopt;
  blink::Vector<SkFontParameters::Variation::Axis> axes;
  axes.resize(axes_count);
  int axes_read =
      base_typeface->getVariationDesignParameters(axes);
  if (axes_read <= 0)
    return std::nullopt;
  for (auto& axis : axes) {
    if (axis.tag == tag) {
      return axis;
    }
  }
  return std::nullopt;
}

}  // namespace

namespace blink {

FontCustomPlatformData::FontCustomPlatformData(PassKey,
                                               sk_sp<SkTypeface> typeface,
                                               size_t data_size)
    : base_typeface_(std::move(typeface)), data_size_(data_size) {
  // The decoded font's SkData is invisible to the garbage collector. It
  // used to be reported to V8 as external memory so that its heap growth
  // heuristics would account for it; V8 no longer owns the heap these
  // objects live in and cppgc has no equivalent hook.
}

FontCustomPlatformData::~FontCustomPlatformData() = default;

const FontPlatformData* FontCustomPlatformData::GetFontPlatformData(
    float size,
    float adjusted_specified_size,
    bool bold,
    bool italic,
    const FontSelectionRequest& selection_request,
    const FontSelectionCapabilities& selection_capabilities,
    const OpticalSizing& optical_sizing,
    TextRenderingMode text_rendering,
    const ResolvedFontFeatures& resolved_font_features,
    FontOrientation orientation,
    const FontVariationSettings* variation_settings,
    const FontPalette* palette) const {
  DCHECK(base_typeface_);

  sk_sp<SkTypeface> return_typeface = base_typeface_;

  // Maximum axis count is maximum value for the OpenType USHORT,
  // which is a 16bit unsigned.
  // https://www.microsoft.com/typography/otspec/fvar.htm Variation
  // settings coming from CSS can have duplicate assignments and the
  // list can be longer than UINT16_MAX, but ignoring the length for
  // now, going with a reasonable upper limit. Deduplication is
  // handled by Skia with priority given to the last occuring
  // assignment.
  FontFormatCheck::VariableFontSubType font_sub_type =
      FontFormatCheck::ProbeVariableFont(base_typeface_);
  bool synthetic_bold = bold;
  bool synthetic_italic = italic;
  if (font_sub_type ==
          FontFormatCheck::VariableFontSubType::kVariableTrueType ||
      font_sub_type == FontFormatCheck::VariableFontSubType::kVariableCFF2) {
    Vector<SkFontArguments::VariationPosition::Coordinate, 0> variation;

    SkFontArguments::VariationPosition::Coordinate weight_coordinate = {
        kWghtTag, SkFloatToScalar(selection_capabilities.weight.clampToRange(
                      selection_request.weight))};
    std::optional<SkFontParameters::Variation::Axis> wght_parameters =
        RetrieveVariationDesignParametersByTag(base_typeface_, kWghtTag);
    if (selection_capabilities.weight.IsRangeSetFromAuto() && wght_parameters) {
      FontSelectionRange wght_range = {
          FontSelectionValue(wght_parameters->min),
          FontSelectionValue(wght_parameters->max)};
      if (wght_range.IsValid()) {
        weight_coordinate = {
            kWghtTag,
            SkFloatToScalar(wght_range.clampToRange(selection_request.weight))};
        bool has_bold_variations = wght_range.maximum > kNormalWeightValue;
        synthetic_bold = bold && !has_bold_variations &&
                         selection_request.weight >= kBoldThreshold;
      }
    }

    SkFontArguments::VariationPosition::Coordinate width_coordinate = {
        kWdthTag, SkFloatToScalar(selection_capabilities.width.clampToRange(
                      selection_request.width))};
    std::optional<SkFontParameters::Variation::Axis> wdth_parameters =
        RetrieveVariationDesignParametersByTag(base_typeface_, kWdthTag);
    if (selection_capabilities.width.IsRangeSetFromAuto() && wdth_parameters) {
      FontSelectionRange wdth_range = {
          FontSelectionValue(wdth_parameters->min),
          FontSelectionValue(wdth_parameters->max)};
      if (wdth_range.IsValid()) {
        width_coordinate = {
            kWdthTag,
            SkFloatToScalar(wdth_range.clampToRange(selection_request.width))};
      }
    }
    // CSS and OpenType have opposite definitions of direction of slant
    // angle. In OpenType positive values turn counter-clockwise, negative
    // values clockwise - in CSS positive values are clockwise rotations /
    // skew. See note in https://drafts.csswg.org/css-fonts/#font-style-prop -
    // map value from CSS to OpenType here.
    SkFontArguments::VariationPosition::Coordinate slant_coordinate = {
        kSlntTag, SkFloatToScalar(-selection_capabilities.slope.clampToRange(
                      selection_request.slope))};
    std::optional<SkFontParameters::Variation::Axis> slnt_parameters =
        RetrieveVariationDesignParametersByTag(base_typeface_, kSlntTag);
    if (selection_capabilities.slope.IsRangeSetFromAuto() && slnt_parameters) {
      FontSelectionRange slnt_range = {
          FontSelectionValue(slnt_parameters->min),
          FontSelectionValue(slnt_parameters->max)};
      if (slnt_range.IsValid()) {
        slant_coordinate = {
            kSlntTag,
            SkFloatToScalar(slnt_range.clampToRange(-selection_request.slope))};
        bool has_right_slanted_variations =
            slnt_range.minimum < kNormalSlopeValue;
        synthetic_italic = italic && !has_right_slanted_variations &&
                           selection_request.slope >= kItalicSlopeValue;
      }
    }

    variation.push_back(weight_coordinate);
    variation.push_back(width_coordinate);
    variation.push_back(slant_coordinate);

    bool explicit_opsz_configured = false;
    if (variation_settings && variation_settings->size() < UINT16_MAX) {
      variation.reserve(variation_settings->size() + variation.size());
      for (const auto& setting : *variation_settings) {
        if (setting.Tag() == kOpszTag)
          explicit_opsz_configured = true;
        SkFontArguments::VariationPosition::Coordinate setting_coordinate =
            {setting.Tag(), SkFloatToScalar(setting.Value())};
        variation.push_back(setting_coordinate);
      }
    }

    if (!explicit_opsz_configured) {
      if (optical_sizing == kAutoOpticalSizing) {
        SkFontArguments::VariationPosition::Coordinate opsz_coordinate = {
            kOpszTag, SkFloatToScalar(adjusted_specified_size)};
        variation.push_back(opsz_coordinate);
      } else if (optical_sizing == kNoneOpticalSizing) {
        // Explicitly set default value to avoid automatic application of
        // optical sizing as it seems to happen on SkTypeface on Mac.
        std::optional<SkFontParameters::Variation::Axis> opsz_parameters =
            RetrieveVariationDesignParametersByTag(return_typeface, kOpszTag);
        if (opsz_parameters) {
          float opszDefault = opsz_parameters->def;
          SkFontArguments::VariationPosition::Coordinate opsz_coordinate = {
              kOpszTag, SkFloatToScalar(opszDefault)};
          variation.push_back(opsz_coordinate);
        }
      }
    }

    SkFontArguments font_args;
    font_args.setVariationDesignPosition(
        {variation.data(), static_cast<int>(variation.size())});
    sk_sp<SkTypeface> sk_variation_font(base_typeface_->makeClone(font_args));

    if (sk_variation_font) {
      return_typeface = sk_variation_font;
    } else {
      SkString family_name;
      base_typeface_->getFamilyName(&family_name);
      // TODO: Surface this as a console message?
      LOG(ERROR) << "Unable for apply variation axis properties for font: "
                 << family_name.c_str();
    }
  }

  if (palette && !palette->IsNormalPalette()) {
    // TODO: Check applicability of font-palette-values according to matching
    // font family name, or should that be done at the CSS family level?

    SkFontArguments font_args;
    SkFontArguments::Palette sk_palette{0, nullptr, 0};

    Vector<FontPalette::FontPaletteOverride> color_overrides;
    std::optional<uint16_t> palette_index = std::nullopt;
    PaletteInterpolation palette_interpolation(base_typeface_);
    if (palette->IsInterpolablePalette()) {
      color_overrides =
          palette_interpolation.ComputeInterpolableFontPalette(palette);
      palette_index = 0;
    } else {
      color_overrides = *palette->GetColorOverrides();
      palette_index = palette_interpolation.RetrievePaletteIndex(palette);
    }

    base::HeapArray<SkFontArguments::Palette::Override> sk_overrides;
    if (palette_index.has_value()) {
      sk_palette.index = *palette_index;

      if (color_overrides.size()) {
        sk_overrides =
            base::HeapArray<SkFontArguments::Palette::Override>::Uninit(
                color_overrides.size());
        for (wtf_size_t i = 0; i < color_overrides.size(); i++) {
          SkColor sk_color = color_overrides[i].color.toSkColor4f().toSkColor();
          sk_overrides[i] = {color_overrides[i].index, sk_color};
        }
        sk_palette.overrides = sk_overrides.data();
        sk_palette.overrideCount = color_overrides.size();
      }

      font_args.setPalette(sk_palette);
    }

    sk_sp<SkTypeface> palette_typeface(return_typeface->makeClone(font_args));
    if (palette_typeface) {
      return_typeface = palette_typeface;
    }
  }
  return MakeGarbageCollected<FontPlatformData>(
      std::move(return_typeface), std::string(), size,
      synthetic_bold && !base_typeface_->isBold(),
      synthetic_italic && !base_typeface_->isItalic(), text_rendering,
      resolved_font_features, orientation);
}

Vector<VariationAxis> FontCustomPlatformData::GetVariationAxes() const {
  return VariableAxesNames::GetVariationAxes(base_typeface_);
}

String FontCustomPlatformData::FamilyNameForInspector() const {
  SkTypeface::LocalizedStrings* font_family_iterator =
      base_typeface_->createFamilyNameIterator();
  SkTypeface::LocalizedString localized_string;
  while (font_family_iterator->next(&localized_string)) {
    // BCP 47 tags for English take precedent in font matching over other
    // localizations: https://drafts.csswg.org/css-fonts/#descdef-src.
    if (localized_string.fLanguage.equals("en") ||
        localized_string.fLanguage.equals("en-US")) {
      break;
    }
  }
  font_family_iterator->unref();
  return String::FromUtf8(base::as_byte_span(localized_string.fString));
}

String FontCustomPlatformData::GetPostScriptNameOrFamilyNameForInspector()
    const {
  SkString postscript_name;
  bool success = base_typeface_->getPostScriptName(&postscript_name);
  if (!success) {
    return FamilyNameForInspector();
  }

  return String(base::as_byte_span(postscript_name));
}

namespace {

// Decoded web fonts by content, kept across documents.
//
// A web font is decoded -- WOFF2 inflated, sanitised by OTS, wrapped in an
// SkTypeface -- when the FontResource that fetched it is first used, and
// the FontResource belongs to a document. A local file is never fresh
// (FreshnessLifetime() returns zero for file: on desktop, so that an edited
// file is seen), so the next document that uses the same file fetches and
// decodes it again: 2.7 ms for a 63 KB Roboto on the development host, half
// of what that page took to render, paid by every capture of a service that
// renders one template with one font all day. The bytes are re-read either
// way, which is what keeps an edited file honest; what this keeps is the
// decode, keyed by what the bytes are rather than where they came from, so
// a changed file is a miss. The typeface is immutable and refcounted and is
// shared the way FontCache shares system typefaces.
//
// Bounded by decoded size, oldest out first, and cleared by an explicit
// release of memory (ClearDecodedFontCache).
class DecodedFontCache {
 public:
  struct Key {
    std::array<uint8_t, crypto::hash::kSha256Size> digest;
    size_t size = 0;
    bool operator==(const Key&) const = default;
  };
  struct Entry {
    Key key;
    sk_sp<SkTypeface> typeface;
    size_t decoded_size = 0;
  };

  static DecodedFontCache& Get() {
    DEFINE_THREAD_SAFE_STATIC_LOCAL(DecodedFontCache, cache, ());
    return cache;
  }

  static Key KeyFor(const SegmentedBuffer& buffer) {
    Key key;
    key.size = buffer.size();
    crypto::hash::Hasher hasher(crypto::hash::HashKind::kSha256);
    for (base::span<const char> segment : buffer) {
      hasher.Update(base::as_bytes(segment));
    }
    hasher.Finish(key.digest);
    return key;
  }

  std::optional<Entry> Find(const Key& key) {
    base::AutoLock lock(lock_);
    for (const Entry& entry : entries_) {
      if (entry.key == key) {
        return entry;
      }
    }
    return std::nullopt;
  }

  void Insert(Entry entry) {
    base::AutoLock lock(lock_);
    if (entry.decoded_size > kMaxBytes) {
      return;
    }
    while (!entries_.empty() && bytes_ + entry.decoded_size > kMaxBytes) {
      bytes_ -= entries_.front().decoded_size;
      entries_.pop_front();
    }
    bytes_ += entry.decoded_size;
    entries_.push_back(std::move(entry));
  }

  void Clear() {
    base::AutoLock lock(lock_);
    entries_.clear();
    bytes_ = 0;
  }

 private:
  // Twenty-four megabytes of decoded font: a few CJK families, or a few
  // hundred subsets.
  static constexpr size_t kMaxBytes = 24u << 20;

  base::Lock lock_;
  Deque<Entry> entries_ GUARDED_BY(lock_);
  size_t bytes_ GUARDED_BY(lock_) = 0;
};

}  // namespace

// static
void FontCustomPlatformData::ClearDecodedFontCache() {
  DecodedFontCache::Get().Clear();
}

FontCustomPlatformData* FontCustomPlatformData::Create(
    SharedBuffer* buffer,
    String& ots_parse_message) {
  DCHECK(buffer);
  const DecodedFontCache::Key key = DecodedFontCache::KeyFor(*buffer);
  if (std::optional<DecodedFontCache::Entry> hit =
          DecodedFontCache::Get().Find(key)) {
    return Create(std::move(hit->typeface), hit->decoded_size);
  }
  base::expected<DecodedWebFont, String> decode_result =
      DecodedWebFont::Create(buffer);
  if (!decode_result.has_value()) {
    ots_parse_message = std::move(decode_result).error();
    return nullptr;
  }
  DecodedFontCache::Get().Insert(
      {key, decode_result->sk_typeface, decode_result->decoded_size});
  return Create(std::move(decode_result->sk_typeface),
                decode_result->decoded_size);
}

FontCustomPlatformData* FontCustomPlatformData::Create(
    sk_sp<SkTypeface> typeface,
    size_t data_size) {
  return MakeGarbageCollected<FontCustomPlatformData>(
      PassKey(), std::move(typeface), data_size);
}

}  // namespace blink
