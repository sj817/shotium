// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/events/add_event_listener_options_resolved.h"

#include "third_party/blink/renderer/core/dom/abort_signal.h"

namespace blink {

// AddEventListenerOptions::Trace() was defined here while the dictionary was
// hand-written. It is generated again, so its definition lives in
// bindings/core/v8/v8_add_event_listener_options.cc and defining it here too
// would be a duplicate symbol.

AddEventListenerOptionsResolved::AddEventListenerOptionsResolved() {}

AddEventListenerOptionsResolved::AddEventListenerOptionsResolved(
    const AddEventListenerOptions* options) {
  DCHECK(options);
  // AddEventListenerOptions
  if (options->hasPassive())
    setPassive(options->passive());
  if (options->hasOnce())
    setOnce(options->once());
  if (options->hasSignal())
    setSignal(options->signal());
  // EventListenerOptions
  if (options->hasCapture())
    setCapture(options->capture());
}

AddEventListenerOptionsResolved::~AddEventListenerOptionsResolved() = default;

void AddEventListenerOptionsResolved::Trace(Visitor* visitor) const {
  AddEventListenerOptions::Trace(visitor);
}

}  // namespace blink
