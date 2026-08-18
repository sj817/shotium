// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/universal_global_scope.h"

#include "base/containers/span.h"
#include "third_party/blink/renderer/core/execution_context/agent.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/base64.h"
#include "third_party/blink/renderer/platform/wtf/text/string_utf8_adaptor.h"

namespace blink {

String UniversalGlobalScope::btoa(const String& string_to_encode,
                                  ExceptionState& exception_state) {
  if (string_to_encode.IsNull()) {
    return String();
  }

  if (!string_to_encode.ContainsOnlyLatin1OrEmpty()) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidCharacterError,
        "The string to be encoded contains "
        "characters outside of the Latin1 range.");
    return String();
  }

  return Base64Encode(base::as_byte_span(string_to_encode.Latin1()));
}

String UniversalGlobalScope::atob(const String& encoded_string,
                                  ExceptionState& exception_state) {
  if (encoded_string.IsNull()) {
    return String();
  }

  if (!encoded_string.ContainsOnlyLatin1OrEmpty()) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidCharacterError,
        "The string to be decoded contains "
        "characters outside of the Latin1 range.");
    return String();
  }
  Vector<uint8_t> out;
  if (!Base64Decode(encoded_string, out, Base64DecodePolicy::kForgiving)) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidCharacterError,
        "The string to be decoded is not correctly encoded.");
    return String();
  }

  return String(out);
}

// queueMicrotask(VoidFunction) was here. The microtask queue itself is very
// much alive -- blink enqueues its own work on it from animation, mutation
// observers and elsewhere -- but this entry point exists to enqueue a script
// function, and there are none.

void UniversalGlobalScope::Trace(Visitor* visitor) const {
  Supplementable<UniversalGlobalScope>::Trace(visitor);
}

}  // namespace blink
