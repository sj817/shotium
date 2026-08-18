// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/url_pattern/url_pattern_regexp.h"

#include <unicode/parseerr.h>
#include <unicode/regex.h>
#include <unicode/unistr.h>
#include <unicode/utypes.h>

#include <memory>
#include <string_view>

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/numerics/safe_conversions.h"

namespace blink {
namespace url_pattern {

namespace {

icu::UnicodeString ToUnicodeString(const String& string) {
  if (string.empty()) {
    return icu::UnicodeString();
  }
  String copy = string;
  copy.Ensure16Bit();
  return icu::UnicodeString(std::u16string_view(copy.Span16()));
}

String ToWTFString(const icu::UnicodeString& string) {
  if (string.isEmpty()) {
    return g_empty_string;
  }
  // SAFETY: icu::UnicodeString::getBuffer() guarantees the returned pointer
  // is valid for at least length() UChars.
  return String(UNSAFE_BUFFERS(base::span<const UChar>(
      base::unchecked, string.getBuffer(),
      base::checked_cast<size_t>(string.length()))));
}

}  // namespace

URLPatternRegexp::URLPatternRegexp(const String& regexp, bool case_sensitive) {
  UErrorCode status = U_ZERO_ERROR;
  UParseError parse_error = {};
  uint32_t flags = case_sensitive ? 0u : UREGEX_CASE_INSENSITIVE;
  std::unique_ptr<icu::RegexPattern> compiled(icu::RegexPattern::compile(
      ToUnicodeString(regexp), flags, parse_error, status));
  if (U_FAILURE(status) || !compiled) {
    // Left invalid. Component::Compile is the only caller and it reports the
    // failure the same way it reported a V8 compile failure; unlike
    // ScriptRegexp we do not keep the message, because nothing read it: the
    // TypeError it built named the offending pattern, not the engine's
    // complaint.
    return;
  }
  pattern_ = std::move(compiled);
}

URLPatternRegexp::~URLPatternRegexp() = default;

bool URLPatternRegexp::Match(StringView input,
                             Vector<String>* group_list) const {
  if (!pattern_ || input.IsNull()) {
    return false;
  }

  UErrorCode status = U_ZERO_ERROR;
  // `subject` is declared before the matcher so that it outlives it:
  // RegexMatcher::reset() keeps a reference to the string rather than copying.
  icu::UnicodeString subject = ToUnicodeString(input.ToString());
  std::unique_ptr<icu::RegexMatcher> matcher(pattern_->matcher(status));
  if (U_FAILURE(status) || !matcher) {
    return false;
  }
  matcher->reset(subject);

  // See the header: the expression is anchored at both ends, so "matches
  // somewhere" and "matches entirely" are the same question, and asking the
  // entire-region form is what keeps ICU's `$` from matching before a trailing
  // newline the way ECMAScript's would not.
  UBool matched = matcher->matches(status);
  if (U_FAILURE(status) || !matched) {
    return false;
  }

  if (group_list) {
    DCHECK(group_list->empty());
    int32_t group_count = matcher->groupCount();
    group_list->ReserveInitialCapacity(
        base::checked_cast<wtf_size_t>(group_count));
    for (int32_t i = 1; i <= group_count; ++i) {
      UErrorCode group_status = U_ZERO_ERROR;
      int32_t start = matcher->start(i, group_status);
      if (U_FAILURE(group_status) || start < 0) {
        // The group did not participate in the match. Upstream got a null
        // String here too, from an `undefined` array element.
        group_list->push_back(String());
        continue;
      }
      icu::UnicodeString group = matcher->group(i, group_status);
      if (U_FAILURE(group_status)) {
        group_list->push_back(String());
        continue;
      }
      group_list->push_back(ToWTFString(group));
    }
  }
  return true;
}

}  // namespace url_pattern
}  // namespace blink
