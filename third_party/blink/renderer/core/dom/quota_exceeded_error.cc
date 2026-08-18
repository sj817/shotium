// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/quota_exceeded_error.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_quota_exceeded_error_options.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/platform/bindings/exception_code.h"

namespace blink {

// static
QuotaExceededError* QuotaExceededError::Create(
    const String& message,
    std::optional<double> quota,
    std::optional<double> requested) {
  return MakeGarbageCollected<QuotaExceededError>(message, quota, requested);
}

// static
void QuotaExceededError::Throw(ExceptionState& exception_state,
                               const String& message) {
  exception_state.ThrowDOMException(DOMExceptionCode::kQuotaExceededError,
                                    message);
}

QuotaExceededError::QuotaExceededError(const String& message,
                                       std::optional<double> quota,
                                       std::optional<double> requested)
    : DOMException(DOMExceptionCode::kQuotaExceededError, message),
      quota_(quota),
      requested_(requested) {}

QuotaExceededError::QuotaExceededError(const String& message)
    : DOMException(DOMExceptionCode::kQuotaExceededError, message) {}

}  // namespace blink
