/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/events/error_event.h"

#include "third_party/blink/renderer/core/event_interface_names.h"
#include "third_party/blink/renderer/core/event_type_names.h"

namespace blink {

ErrorEvent* ErrorEvent::CreateSanitizedError() {
  // "6. If script's muted errors is true, then set message to "Script error.",
  // urlString to the empty string, line and col to 0, and errorValue to null."
  // https://html.spec.whatwg.org/C/#runtime-script-errors:muted-errors
  return MakeGarbageCollected<ErrorEvent>(
      "Script error.",
      MakeGarbageCollected<SourceLocation>(String(), String(), 0, 0));
}

ErrorEvent::ErrorEvent(const String& message, SourceLocation* location)
    : Event(event_type_names::kError, Bubbles::kNo, Cancelable::kYes),
      sanitized_message_(message),
      location_(location) {}

void ErrorEvent::SetUnsanitizedMessage(const String& message) {
  DCHECK(unsanitized_message_.empty());
  unsanitized_message_ = message;
}

ErrorEvent::~ErrorEvent() = default;

const AtomicString& ErrorEvent::InterfaceName() const {
  return event_interface_names::kErrorEvent;
}

bool ErrorEvent::IsErrorEvent() const {
  return true;
}

void ErrorEvent::Trace(Visitor* visitor) const {
  visitor->Trace(location_);
  Event::Trace(visitor);
}

}  // namespace blink
