/*
 * Copyright (C) 2007 Henry Mason (hmason@mac.com)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights
 * reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_MESSAGE_EVENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_MESSAGE_EVENT_H_

#include <memory>

#include "base/check_op.h"
#include "third_party/blink/public/mojom/messaging/delegated_capability.mojom-blink.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/fileapi/blob.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/core/url/dom_origin_utils.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"

namespace blink {

class CORE_EXPORT MessageEvent final : public Event {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum MessageOriginKind {
    kMessageIsSameOrigin,
    kMessageIsCrossOrigin,
  };

  static MessageEvent* Create() { return MakeGarbageCollected<MessageEvent>(); }
  static MessageEvent* CreateError() {
    scoped_refptr<const SecurityOrigin> nullptr_origin;
    return MakeGarbageCollected<MessageEvent>(std::move(nullptr_origin),
                                              nullptr);
  }
  static MessageEvent* CreateError(const MessageEvent* event) {
    return MakeGarbageCollected<MessageEvent>(event->GetSecurityOrigin(),
                                              event->source());
  }
  static MessageEvent* CreateError(const SecurityOrigin* origin,
                                   EventTarget* source = nullptr) {
    return MakeGarbageCollected<MessageEvent>(origin, source);
  }
  static MessageEvent* Create(
      const String& data,
      scoped_refptr<const SecurityOrigin> origin = nullptr) {
    return MakeGarbageCollected<MessageEvent>(data, std::move(origin));
  }
  static MessageEvent* Create(
      Blob* data,
      scoped_refptr<const SecurityOrigin> origin = nullptr) {
    return MakeGarbageCollected<MessageEvent>(data, std::move(origin));
  }
  static MessageEvent* Create(
      DOMArrayBuffer* data,
      scoped_refptr<const SecurityOrigin> origin = nullptr) {
    return MakeGarbageCollected<MessageEvent>(data, std::move(origin));
  }
  MessageEvent();
  // Creates a "messageerror" event.
  MessageEvent(scoped_refptr<const SecurityOrigin> origin, EventTarget* source);
  MessageEvent(const String& data, scoped_refptr<const SecurityOrigin> origin);
  MessageEvent(Blob* data, scoped_refptr<const SecurityOrigin> origin);
  MessageEvent(DOMArrayBuffer* data,
               scoped_refptr<const SecurityOrigin> origin);
  ~MessageEvent() override;

  // DOMOriginUtils overrides:
  DOMOrigin* GetDOMOrigin(LocalDOMWindow*) const override;

  // Not an implementation of the (removed) bindings-exposed `initMessageEvent`
  // method; it re-initialises an already-constructed event in place. The
  // `ports` parameter was dropped along with MessagePort support (see the
  // comment on `ports_` below).
  void initMessageEvent(const AtomicString& type,
                        bool bubbles,
                        bool cancelable,
                        const String& data,
                        scoped_refptr<const SecurityOrigin> origin,
                        const String& last_event_id,
                        EventTarget* source);

  enum DataType {
    kDataTypeNull,  // For "messageerror" events, and for events whose payload
                    // has not been set yet.
    kDataTypeString,
    kDataTypeBlob,
    kDataTypeArrayBuffer
  };

  // The payload the event actually carries. Exactly one of the three accessors
  // below is meaningful, selected by `DataKind()`.
  DataType DataKind() const { return data_type_; }
  const String& DataAsString() const {
    DCHECK_EQ(data_type_, kDataTypeString);
    return data_as_string_;
  }
  Blob* DataAsBlob() const {
    DCHECK_EQ(data_type_, kDataTypeBlob);
    return data_as_blob_.Get();
  }
  DOMArrayBuffer* DataAsArrayBuffer() const {
    DCHECK_EQ(data_type_, kDataTypeArrayBuffer);
    return data_as_array_buffer_.Get();
  }

  const String& lastEventId() const { return last_event_id_; }
  EventTarget* source() const { return source_.Get(); }
  uint64_t GetTraceId() const { return trace_id_; }
  void SetTraceId(uint64_t trace_id) { trace_id_ = trace_id; }

  const AtomicString& InterfaceName() const override;

  // Returns true when the payload contains values that remote origins cannot
  // access. If true, remote origins must dispatch a messageerror event instead
  // of a message event.
  bool IsOriginCheckRequiredToAccessData() const;

  // Returns true when this event is locked to an agent cluster.
  bool IsLockedToAgentCluster() const;

  // Returns true when the payload is not prevented from being read in the
  // provided execution context.
  bool CanDeserializeIn(ExecutionContext*) const;

  void Trace(Visitor*) const override;

  void LockToAgentCluster();

  scoped_refptr<const SecurityOrigin> GetSecurityOrigin() const {
    return origin_;
  }

 private:
  DataType data_type_;
  String data_as_string_;
  Member<Blob> data_as_blob_;
  Member<DOMArrayBuffer> data_as_array_buffer_;

  // We hold a `SecurityOrigin` in `origin_` which we'll use for any and all
  // security checks. We also potentially have to hold a string representing
  // the serialized origin that was handed to us. See
  // https://github.com/whatwg/html/issues/11759 for discussion.
  scoped_refptr<const SecurityOrigin> origin_;
  String potentially_invalid_origin_serialization_;

  String last_event_id_;
  Member<EventTarget> source_;
  // ports_/channels_ (the MessagePorts in an entangled state, and the
  // MessageChannels in a disentangled state that EntangleMessagePorts()
  // converted between) were removed along with MessagePort support: no
  // worker/channel-messaging infrastructure exists to originate either one
  // (see core/messaging, deleted).
  // For messages that crossed a process boundary this records whether the
  // original message was locked to the sender's agent cluster.
  bool locked_to_agent_cluster_ = false;
  uint64_t trace_id_ = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EVENTS_MESSAGE_EVENT_H_
