/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "third_party/blink/renderer/core/events/message_event.h"

#include <memory>

#include "third_party/blink/public/mojom/use_counter/metrics/web_feature.mojom-blink.h"
#include "third_party/blink/renderer/core/event_interface_names.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/url/dom_origin.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

static inline bool IsValidSource(EventTarget* source) {
  return !source || source->ToDOMWindow() || source->ToMessagePort() ||
         source->ToServiceWorker();
}

DOMOrigin* MessageEvent::GetDOMOrigin(LocalDOMWindow*) const {
  // We only create `DOMOrigin` objects for `MessageEvent` objects that were not
  // constructed from JavaScript, as the JavaScript constructor accepts an
  // untrusted string serialization of an origin.
  if (!potentially_invalid_origin_serialization_.IsNull() ||
      !GetSecurityOrigin()) {
    return nullptr;
  }

  // No access check is required, as this object intentionally reveals its
  // sender's origin cross-origin.
  return DOMOrigin::Create(GetSecurityOrigin());
}

MessageEvent::MessageEvent() : data_type_(kDataTypeNull) {}

MessageEvent::MessageEvent(scoped_refptr<const SecurityOrigin> origin,
                           EventTarget* source)
    : Event(event_type_names::kMessageerror, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeNull),
      origin_(std::move(origin)),
      source_(source) {
  DCHECK(IsValidSource(source_.Get()));
}

MessageEvent::MessageEvent(const String& data,
                           scoped_refptr<const SecurityOrigin> origin)
    : Event(event_type_names::kMessage, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeString),
      data_as_string_(data),
      origin_(std::move(origin)) {}

MessageEvent::MessageEvent(Blob* data,
                           scoped_refptr<const SecurityOrigin> origin)
    : Event(event_type_names::kMessage, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeBlob),
      data_as_blob_(data),
      origin_(std::move(origin)) {}

MessageEvent::MessageEvent(DOMArrayBuffer* data,
                           scoped_refptr<const SecurityOrigin> origin)
    : Event(event_type_names::kMessage, Bubbles::kNo, Cancelable::kNo),
      data_type_(kDataTypeArrayBuffer),
      data_as_array_buffer_(data),
      origin_(std::move(origin)) {}

MessageEvent::~MessageEvent() = default;

void MessageEvent::initMessageEvent(const AtomicString& type,
                                    bool bubbles,
                                    bool cancelable,
                                    const String& data,
                                    scoped_refptr<const SecurityOrigin> origin,
                                    const String& last_event_id,
                                    EventTarget* source) {
  if (IsBeingDispatched())
    return;

  initEvent(type, bubbles, cancelable);

  data_type_ = kDataTypeString;
  data_as_string_ = data;
  origin_ = std::move(origin);
  last_event_id_ = last_event_id;
  source_ = source;
}

const AtomicString& MessageEvent::InterfaceName() const {
  return event_interface_names::kMessageEvent;
}

bool MessageEvent::IsOriginCheckRequiredToAccessData() const {
  // Only a structured-clone payload could carry values (SharedArrayBuffer,
  // WasmModule, ...) whose exposure has to be gated on the receiver's origin.
  // MessageEvent can no longer hold one: every payload kind it still supports
  // (none / string / Blob / ArrayBuffer) is readable cross-origin.
  return false;
}

bool MessageEvent::IsLockedToAgentCluster() const {
  // The payload itself can no longer pin the event to an agent cluster; only
  // an explicit LockToAgentCluster() call by the sender can.
  return locked_to_agent_cluster_;
}

bool MessageEvent::CanDeserializeIn(ExecutionContext*) const {
  // Only a structured-clone payload could be restricted to a particular
  // execution context, and MessageEvent no longer carries one.
  return true;
}

void MessageEvent::Trace(Visitor* visitor) const {
  visitor->Trace(data_as_blob_);
  visitor->Trace(data_as_array_buffer_);
  visitor->Trace(source_);
  Event::Trace(visitor);
}

void MessageEvent::LockToAgentCluster() {
  locked_to_agent_cluster_ = true;
}

}  // namespace blink
