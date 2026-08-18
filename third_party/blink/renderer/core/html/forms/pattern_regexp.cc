// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/html/forms/pattern_regexp.h"

#include <unicode/parseerr.h>
#include <unicode/regex.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

#include <memory>
#include <string_view>

#include "third_party/blink/renderer/platform/wtf/text/strcat.h"
#include "third_party/blink/renderer/platform/wtf/text/string_view.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

icu::UnicodeString ToUnicodeString(const String& string) {
  if (string.empty()) {
    return icu::UnicodeString();
  }
  String copy = string;
  copy.Ensure16Bit();
  return icu::UnicodeString(std::u16string_view(copy.Span16()));
}

}  // namespace

PatternRegexp::PatternRegexp(const String& pattern) {
  UErrorCode status = U_ZERO_ERROR;
  UParseError parse_error = {};
  std::unique_ptr<icu::RegexPattern> compiled(icu::RegexPattern::compile(
      ToUnicodeString(pattern), /*flags=*/0, parse_error, status));
  if (U_FAILURE(status) || !compiled) {
    exception_message_ =
        StrCat({"invalid regular expression at offset ",
                String::Number(parse_error.offset), " (",
                StringView(u_errorName(status)), ")"});
    return;
  }
  pattern_ = std::move(compiled);
}

PatternRegexp::~PatternRegexp() = default;

bool PatternRegexp::MatchesEntirely(const String& value) const {
  if (!pattern_) {
    return false;
  }
  UErrorCode status = U_ZERO_ERROR;
  // `input` is declared first so that it outlives the matcher:
  // RegexMatcher::reset() keeps a reference to it rather than copying.
  icu::UnicodeString input = ToUnicodeString(value);
  std::unique_ptr<icu::RegexMatcher> matcher(pattern_->matcher(status));
  if (U_FAILURE(status) || !matcher) {
    return false;
  }
  matcher->reset(input);
  // RegexMatcher::matches() requires the whole input region to be consumed,
  // which is exactly the anchoring the `pattern` attribute specifies.
  UBool matched = matcher->matches(status);
  return U_SUCCESS(status) && matched;
}

}  // namespace blink
