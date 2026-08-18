// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_QUOTA_EXCEEDED_ERROR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_QUOTA_EXCEEDED_ERROR_H_

#include <optional>

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"

namespace blink {

class CORE_EXPORT QuotaExceededError : public DOMException {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // For creating a QuotaExceededError from C++.
  static QuotaExceededError* Create(const String& message,
                                    std::optional<double> quota,
                                    std::optional<double> requested);

  // For throwing a QuotaExceededError from ExceptionState.
  static void Throw(ExceptionState& exception_state, const String& message);

  QuotaExceededError(const String& message,
                     std::optional<double> quota,
                     std::optional<double> requested);
  explicit QuotaExceededError(const String& message);

  std::optional<double> quota() const { return quota_; }
  std::optional<double> requested() const { return requested_; }

 private:
  std::optional<double> quota_;
  std::optional<double> requested_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_QUOTA_EXCEEDED_ERROR_H_
