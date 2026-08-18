// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_URL_PATTERN_URL_PATTERN_REGEXP_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_URL_PATTERN_URL_PATTERN_REGEXP_H_

#include <memory>

#include "third_party/blink/renderer/platform/wtf/text/string_view.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

// `icu` is not a namespace: <unicode/uversion.h> declares the real namespace as
// icu_78 (U_ICU_NAMESPACE, the version-renamed entry point) and then adds
// `namespace icu = icu_78;`. Opening `namespace icu { ... }` by hand declares a
// *second*, real namespace with that name, and from then on `icu::UnicodeString`
// does not resolve -- not in this file, and not inside ICU's own headers, which
// is where the diagnostics actually appear. U_NAMESPACE_BEGIN/END expand to the
// renamed namespace, which is what a forward declaration has to use.
#include <unicode/uversion.h>

U_NAMESPACE_BEGIN
class RegexPattern;
U_NAMESPACE_END

namespace blink {
namespace url_pattern {

// The regular expression a url_pattern::Component falls back to when the
// parsed pattern cannot be matched directly (`liburlpattern::Pattern::
// CanDirectMatch()` is false, i.e. the pattern has a custom `(regex)` group or
// a repeated/optional group).
//
// This used to be platform/bindings/script_regexp.h, which compiled the string
// that `liburlpattern::Pattern::GenerateRegexString()` produces with V8's
// RegExp engine. There is no script engine, so it is compiled with ICU's regex
// engine instead, the same substitution core/html/forms/pattern_regexp.h makes
// for the `pattern` content attribute. Two behavioural notes, both inherent to
// the swap rather than to this class:
//
//  * ICU implements the same Perl-derived syntax ECMAScript regular
//    expressions use, but not the `v` flag's set notation. Upstream compiled
//    with UnicodeMode::kUnicodeSets, so a pattern whose custom regexp group
//    relies on nested character-class set operations fails to compile here
//    where V8 accepted it. `IsValid()` reports that, and Component::Compile
//    turns it into the same "custom regular expression group is invalid"
//    TypeError it already raised for a group V8 rejected.
//  * Case-insensitive matching (`URLPatternOptions.ignoreCase`) uses ICU's
//    full Unicode case folding. V8 was asked for the plain `i` flag, which is
//    also not ASCII-only, so the two agree on everything the URL
//    canonicalisation upstream of here can actually hand us: by the time a
//    value reaches Match() it has been percent-encoded to ASCII.
class URLPatternRegexp final {
 public:
  // `regexp` is a regular expression string from GenerateRegexString().
  URLPatternRegexp(const String& regexp, bool case_sensitive);
  URLPatternRegexp(const URLPatternRegexp&) = delete;
  URLPatternRegexp& operator=(const URLPatternRegexp&) = delete;
  ~URLPatternRegexp();

  bool IsValid() const { return pattern_ != nullptr; }

  // Attempt to match `input` against the regular expression. Returns true on a
  // match. If `group_list` is provided it is populated with the values of the
  // capture groups -- the values normally at index 1 and up in the array
  // RegExp.prototype.exec() returned, which is the contract Component::Match
  // was written against. A group that did not participate in the match yields
  // a null String, the way an `undefined` element did.
  //
  // Unlike ScriptRegexp::Match this returns a bool rather than a match offset:
  // the only caller compared the offset against 0, and GenerateRegexString()
  // anchors every expression it emits with `^`...`$` (url_pattern never
  // overrides liburlpattern::Options::start/end/ends_with), so a match can
  // only ever be at 0. Requiring the whole input to be consumed is also how
  // ECMAScript's unanchored-`$` semantics are reproduced: ICU's `$`, unlike
  // V8's, would otherwise also match before a trailing line terminator.
  bool Match(StringView input, Vector<String>* group_list) const;

 private:
  std::unique_ptr<icu::RegexPattern> pattern_;
};

}  // namespace url_pattern
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_URL_PATTERN_URL_PATTERN_REGEXP_H_
