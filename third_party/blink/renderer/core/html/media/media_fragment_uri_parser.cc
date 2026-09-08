/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/media/media_fragment_uri_parser.h"

#include <string_view>

#include "base/check_op.h"
#include "base/containers/adapters.h"
#include "base/strings/string_number_conversions.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

constexpr std::string_view kPixelIdentifier = "pixel:";
constexpr std::string_view kPercentIdentifier = "percent:";

// Consumes leading ASCII digits from `str`, converts them to an int via
// base::StringToInt, and advances `str` past the digits.
bool ParseNonNegativeInt(std::string_view& str, int& out) {
  size_t i = 0;
  while (i < str.size() && IsAsciiDigit(str[i])) {
    ++i;
  }
  if (!base::StringToInt(str.substr(0, i), &out)) {
    return false;
  }
  str.remove_prefix(i);
  return true;
}

// Consumes a single comma from `str`. Returns false if `str` does not start
// with ','.
bool ParseComma(std::string_view& str) {
  if (!str.starts_with(',')) {
    return false;
  }
  str.remove_prefix(1);
  return true;
}

}  // namespace

MediaFragmentURIParser::MediaFragmentURIParser(const StringView& fragment)
    : fragment_(fragment.ToString()) {}

void MediaFragmentURIParser::ParseFragments() {
  has_parsed_fragments_ = true;
  StringView fragment_string = fragment_;
  if (fragment_string.empty())
    return;

  wtf_size_t offset = 0;
  wtf_size_t end = fragment_string.length();
  while (offset < end) {
    // http://www.w3.org/2008/WebVideo/Fragments/WD-media-fragments-spec/#processing-name-value-components
    // 1. Parse the octet string according to the namevalues syntax, yielding a
    //    list of name-value pairs, where name and value are both octet string.
    //    In accordance with RFC 3986, the name and value components must be
    //    parsed and separated before percent-encoded octets are decoded.
    wtf_size_t parameter_start = offset;
    wtf_size_t parameter_end = fragment_string.find('&', offset);
    if (parameter_end == kNotFound)
      parameter_end = end;

    wtf_size_t equal_offset = fragment_string.find('=', offset);
    if (equal_offset == kNotFound || equal_offset > parameter_end) {
      offset = parameter_end + 1;
      continue;
    }

    // 2. For each name-value pair:
    //  a. Decode percent-encoded octets in name and value as defined by RFC
    //     3986. If either name or value are not valid percent-encoded strings,
    //     then remove the name-value pair from the list.
    String name = DecodeUrlEscapeSequences(
        fragment_string.substr(parameter_start, equal_offset - parameter_start),
        DecodeUrlMode::kUtf8OrIsomorphic);
    String value;
    if (equal_offset != parameter_end) {
      value = DecodeUrlEscapeSequences(
          fragment_string.substr(equal_offset + 1,
                                 parameter_end - equal_offset - 1),
          DecodeUrlMode::kUtf8OrIsomorphic);
    }

    //  b. Convert name and value to Unicode strings by interpreting them as
    //     UTF-8. If either name or value are not valid UTF-8 strings, then
    //     remove the name-value pair from the list.
    bool valid_utf8 = true;
    std::string utf8_name;
    if (!name.empty()) {
      utf8_name = name.Utf8(Utf8ConversionMode::kStrict);
      valid_utf8 = !utf8_name.empty();
    }
    std::string utf8_value;
    if (valid_utf8 && !value.empty()) {
      utf8_value = value.Utf8(Utf8ConversionMode::kStrict);
      valid_utf8 = !utf8_value.empty();
    }

    if (valid_utf8)
      fragments_.emplace_back(std::move(utf8_name), std::move(utf8_value));

    offset = parameter_end + 1;
  }
}

SpatialClip MediaFragmentURIParser::SpatialFragment() {
  if (fragment_.IsNull()) {
    return {};
  }
  if (!has_parsed_spatial_) {
    ParseSpatialFragment();
  }
  return spatial_clip_;
}

void MediaFragmentURIParser::ParseSpatialFragment() {
  has_parsed_spatial_ = true;
  if (!has_parsed_fragments_) {
    ParseFragments();
  }

  // When a fragment dimension occurs multiple times, only the last
  // valid occurrence of that dimension is used. Iterate in reverse to find it.
  for (const auto& fragment : base::Reversed(fragments_)) {
    // https://www.w3.org/TR/media-frags/#naming-space
    // Spatial clipping is denoted by the name xywh.
    if (fragment.first != "xywh") {
      continue;
    }

    SpatialClip clip = ParseXYWH(fragment.second);
    if (clip.IsValid()) {
      spatial_clip_ = clip;
      break;
    }
  }
}

// TODO(dmangal): Consider separating the syntax parsing and semantic
// validation, if the validation rules becomes complex.
//
// https://www.w3.org/TR/media-frags/#valid-uri-spatial
SpatialClip MediaFragmentURIParser::ParseXYWH(std::string_view value) {
  // https://www.w3.org/TR/media-frags/#naming-space
  // Spatial clipping is denoted by the name xywh. The value is an optional
  // unit prefix (pixel: or percent:) followed by four comma-separated
  // non-negative integers: x, y, w, h. The default unit is pixel.
  //
  // xywhdef   = "xywh=" xywhunit ":" 1*DIGIT "," 1*DIGIT "," 1*DIGIT ","
  //             1*DIGIT
  // xywhunit  = %x70.69.78.65.6C        ; "pixel"
  //           / %x70.65.72.63.65.6E.74   ; "percent"
  SpatialClip::Unit unit = SpatialClip::Unit::kPixel;

  if (value.starts_with(kPixelIdentifier)) {
    unit = SpatialClip::Unit::kPixel;
    value.remove_prefix(kPixelIdentifier.size());
  } else if (value.starts_with(kPercentIdentifier)) {
    unit = SpatialClip::Unit::kPercent;
    value.remove_prefix(kPercentIdentifier.size());
  }

  // Parse four comma-separated non-negative integers: x,y,w,h.
  // x and y must be >= 0, w and h must be > 0.
  int x, y, w, h;
  if (!ParseNonNegativeInt(value, x) || !ParseComma(value) ||
      !ParseNonNegativeInt(value, y) || !ParseComma(value) ||
      !ParseNonNegativeInt(value, w) || w == 0 || !ParseComma(value) ||
      !ParseNonNegativeInt(value, h) || h == 0 || !value.empty()) {
    return {};
  }

  DCHECK_GE(x, 0);
  DCHECK_GE(y, 0);
  DCHECK_GT(w, 0);
  DCHECK_GT(h, 0);

  // For percent coordinates, additionally x+w <= 100 and y+h <= 100.
  if (unit == SpatialClip::Unit::kPercent &&
      (x > 100 || w > 100 || y > 100 || h > 100 || x + w > 100 ||
       y + h > 100)) {
    return {};
  }

  return SpatialClip{gfx::Rect(x, y, w, h), unit};
}

}  // namespace blink
