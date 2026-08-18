// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/abort_controller.h"

#include "third_party/blink/renderer/core/dom/abort_signal.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/platform/bindings/exception_code.h"
#include "third_party/blink/renderer/platform/heap/visitor.h"

namespace blink {

AbortController::AbortController(AbortSignal* signal) : signal_(signal) {}

AbortController::~AbortController() = default;

void AbortController::Dispose() {
  signal_->DetachFromController();
}

void AbortController::abort() {
  abort(MakeGarbageCollected<DOMException>(DOMExceptionCode::kAbortError,
                                           "signal is aborted without reason"));
}

void AbortController::abort(DOMException* reason) {
  CHECK(reason);
  signal_->SignalAbort(reason, AbortSignal::SignalAbortPassKey());
}

void AbortController::Trace(Visitor* visitor) const {
  visitor->Trace(signal_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
