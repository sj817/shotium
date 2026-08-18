// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_PATTERN_REGEXP_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_PATTERN_REGEXP_H_

#include <memory>

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

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

// A compiled `pattern` content attribute
// (https://html.spec.whatwg.org/C/#the-pattern-attribute).
//
// This used to be ScriptRegexp, which compiled the pattern with V8's RegExp
// engine. There is no script engine, so the pattern is compiled with ICU's
// regex engine instead. ICU implements the same Perl-derived syntax that
// ECMAScript regular expressions use; it does not implement the `v` flag's set
// notation, so a pattern relying on nested character-class set operations will
// fail to compile here where V8 would have accepted it. `IsValid()` reports
// that case and callers treat it the way they treated a V8 compile failure.
class CORE_EXPORT PatternRegexp final {
 public:
  // Compiles `pattern` case-sensitively, without multiline.
  explicit PatternRegexp(const String& pattern);
  PatternRegexp(const PatternRegexp&) = delete;
  PatternRegexp& operator=(const PatternRegexp&) = delete;
  ~PatternRegexp();

  bool IsValid() const { return pattern_ != nullptr; }

  // Non-empty only when !IsValid().
  const String& ExceptionMessage() const { return exception_message_; }

  // True when `value` matches the pattern in its entirety, which is the match
  // the `pattern` attribute is defined in terms of. Always false when the
  // pattern failed to compile.
  bool MatchesEntirely(const String& value) const;

 private:
  std::unique_ptr<icu::RegexPattern> pattern_;
  String exception_message_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_FORMS_PATTERN_REGEXP_H_
