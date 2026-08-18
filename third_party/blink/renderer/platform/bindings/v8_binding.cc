// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/bindings/v8_binding.h"

#include "third_party/blink/renderer/platform/wtf/text/character_names.h"
#include "third_party/blink/renderer/platform/wtf/text/string_buffer.h"
#include "third_party/blink/renderer/platform/wtf/text/unicode.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

// ReplaceUnmatchedSurrogates() lived in bindings/core/v8/v8_binding_for_core.cc
// and went with it. It is not a V8 function: it implements
// https://webidl.spec.whatwg.org/#dfn-obtain-unicode over a WTF::String, and
// the caller left in this tree is FormData, which has to apply it to every
// entry name and value it is handed. Moved rather than restored, for the same
// reason the interceptor result enums in the header next to this file were --
// the directory was V8's, the code is not.

namespace blink {

static bool HasUnmatchedSurrogates(const String& string) {
  // By definition, 8-bit strings are confined to the Latin-1 code page and
  // have no surrogates, matched or otherwise.
  if (string.empty() || string.Is8Bit())
    return false;

  auto characters = string.Span16();
  const size_t length = characters.size();

  for (size_t i = 0; i < length; ++i) {
    UChar c = characters[i];
    if (U16_IS_SINGLE(c))
      continue;
    if (U16_IS_TRAIL(c))
      return true;
    DCHECK(U16_IS_LEAD(c));
    if (i == length - 1)
      return true;
    UChar d = characters[i + 1];
    if (!U16_IS_TRAIL(d))
      return true;
    ++i;
  }
  return false;
}

// Replace unmatched surrogates with REPLACEMENT CHARACTER U+FFFD.
String ReplaceUnmatchedSurrogates(String string) {
  // This roughly implements https://webidl.spec.whatwg.org/#dfn-obtain-unicode
  // but since Blink strings are 16-bits internally, the output is simply
  // re-encoded to UTF-16.

  // The concept of surrogate pairs is explained at:
  // http://www.unicode.org/versions/Unicode6.2.0/ch03.pdf#G2630

  // Blink-specific optimization to avoid making an unnecessary copy.
  if (!HasUnmatchedSurrogates(string))
    return string;
  DCHECK(!string.Is8Bit());

  // 1. Let S be the DOMString value.
  const auto s = string.Span16();

  // 2. Let n be the length of S.
  const size_t n = s.size();

  // 3. Initialize i to 0.
  size_t i = 0;

  // 4. Initialize U to be an empty sequence of Unicode characters.
  StringBuffer<UChar> result(static_cast<wtf_size_t>(n));
  auto u = result.Span();

  // 5. While i < n:
  while (i < n) {
    // 1. Let c be the code unit in S at index i.
    UChar c = s[i];
    // 2. Depending on the value of c:
    if (U16_IS_SINGLE(c)) {
      // c < 0xD800 or c > 0xDFFF
      // Append to U the Unicode character with code point c.
      u[i] = c;
    } else if (U16_IS_TRAIL(c)) {
      // 0xDC00 <= c <= 0xDFFF
      // Append to U a U+FFFD REPLACEMENT CHARACTER.
      u[i] = uchar::kReplacementCharacter;
    } else {
      // 0xD800 <= c <= 0xDBFF
      DCHECK(U16_IS_LEAD(c));
      if (i == n - 1) {
        // 1. If i = n-1, then append to U a U+FFFD REPLACEMENT CHARACTER.
        u[i] = uchar::kReplacementCharacter;
      } else {
        // 2. Otherwise, i < n-1:
        DCHECK_LT(i, n - 1);
        // ....1. Let d be the code unit in S at index i+1.
        UChar d = s[i + 1];
        if (U16_IS_TRAIL(d)) {
          // 2. If 0xDC00 <= d <= 0xDFFF, then:
          // ..1. Let a be c & 0x3FF.
          // ..2. Let b be d & 0x3FF.
          // ..3. Append to U the Unicode character with code point
          //      2^16+2^10*a+b.
          u[i++] = c;
          u[i] = d;
        } else {
          // 3. Otherwise, d < 0xDC00 or d > 0xDFFF. Append to U a U+FFFD
          //    REPLACEMENT CHARACTER.
          u[i] = uchar::kReplacementCharacter;
        }
      }
    }
    // 3. Set i to i+1.
    ++i;
  }

  // 6. Return U.
  DCHECK_EQ(i, string.length());
  return String::Adopt(result);
}

}  // namespace blink
