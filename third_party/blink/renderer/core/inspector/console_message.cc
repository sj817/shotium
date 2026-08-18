// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/inspector/console_message.h"

#include <memory>
#include <utility>

#include "base/time/time.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

// ConsoleMessage lived in core/inspector and went with the inspector sweep, but
// it is not DevTools: it is the plain data object every part of blink builds to
// report a problem -- a CSP violation, a parse error, a deprecation, a failed
// subresource. Around 180 call sites in core construct one, and it holds no V8
// type at all (a source, a level, a string, a SourceLocation, a frame and some
// node ids).
//
// Two of the four constructors did not come back:
//   - the WorkerThread one, which tagged a message with the worker's DevTools
//     token via IdentifiersFactory; workers are cut.
//   - the WebConsoleMessage one, which the embedder used to inject a message
//     through the public web API; that API is cut.
// The DocumentLoader constructor came back with its request identifier dropped
// for the same reason -- IdentifiersFactory minted DevTools protocol ids and is
// gone -- so RequestIdentifier() now returns the null string. It is read only by
// the network agent, which is also gone.

namespace blink {

ConsoleMessage::ConsoleMessage(mojom::blink::ConsoleMessageSource source,
                               mojom::blink::ConsoleMessageLevel level,
                               const String& message,
                               const String& url,
                               DocumentLoader* loader,
                               uint64_t request_identifier)
    : ConsoleMessage(source, level, message, CaptureSourceLocation(url, 0, 0)) {
  // request_identifier_ was IdentifiersFactory::RequestId(loader,
  // request_identifier): the id the DevTools protocol used to correlate this
  // message with a Network.requestWillBeSent event.
}

ConsoleMessage::ConsoleMessage(mojom::blink::ConsoleMessageSource source,
                               mojom::blink::ConsoleMessageLevel level,
                               const String& message,
                               SourceLocation* location)
    : source_(source),
      level_(level),
      message_(message),
      location_(location),
      timestamp_(base::Time::Now().InMillisecondsFSinceUnixEpoch()),
      frame_(nullptr) {
  DCHECK(location_);
}

ConsoleMessage::~ConsoleMessage() = default;

SourceLocation* ConsoleMessage::Location() const {
  return location_.Get();
}

const String& ConsoleMessage::RequestIdentifier() const {
  return request_identifier_;
}

double ConsoleMessage::Timestamp() const {
  return timestamp_;
}

ConsoleMessage::Source ConsoleMessage::GetSource() const {
  return source_;
}

ConsoleMessage::Level ConsoleMessage::GetLevel() const {
  return level_;
}

const String& ConsoleMessage::Message() const {
  return message_;
}

LocalFrame* ConsoleMessage::Frame() const {
  // Do not reference detached frames.
  if (frame_ && frame_->Client()) {
    return frame_.Get();
  }
  return nullptr;
}

Vector<DOMNodeId>& ConsoleMessage::Nodes() {
  return nodes_;
}

void ConsoleMessage::SetNodes(LocalFrame* frame, Vector<DOMNodeId> nodes) {
  frame_ = frame;
  nodes_ = std::move(nodes);
}

const std::optional<mojom::blink::ConsoleMessageCategory>&
ConsoleMessage::Category() const {
  return category_;
}

void ConsoleMessage::SetCategory(
    mojom::blink::ConsoleMessageCategory category) {
  category_ = category;
}

void ConsoleMessage::Trace(Visitor* visitor) const {
  visitor->Trace(frame_);
  visitor->Trace(location_);
}

}  // namespace blink
