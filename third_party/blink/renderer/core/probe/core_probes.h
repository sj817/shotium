/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PROBE_CORE_PROBES_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PROBE_CORE_PROBES_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace network {
namespace mojom {
namespace blink {
class WebSocketHandshakeResponse;
class WebSocketHandshakeRequest;
}  // namespace blink
}  // namespace mojom
}  // namespace network

namespace blink {

class CoreProbeSink;

namespace protocol {
namespace Network {
class DirectTCPSocketOptions;
class DirectUDPSocketOptions;
}  // namespace Network
namespace Audits {
class InspectorIssue;
}  // namespace Audits
}  // namespace protocol

namespace probe {

class AsyncTaskContext;

class CORE_EXPORT ProbeBase {
  STACK_ALLOCATED();

 public:
  base::TimeTicks CaptureStartTime() const;
  base::TimeTicks CaptureEndTime() const;
  base::TimeDelta Duration() const;

 private:
  mutable base::TimeTicks start_time_;
  mutable base::TimeTicks end_time_;
};

// Tracks execution of a (previously scheduled) asynchronous task. An instance
// should exist for the full duration of the task's execution.
class CORE_EXPORT AsyncTask {
  STACK_ALLOCATED();

 public:
  // Args:
  //   context: The ExecutionContext in which the task is executed.
  //   task: An identifier for the AsyncTask.
  //   step: A nullptr indicates a task that is not recurring. A non-null value
  //     indicates a recurring task with the value used for tracing events.
  //
  // What survives here is the perfetto flow: the task is traced from where it
  // was scheduled to where it runs. `context` used to select the
  // ScriptInitiationMonitor that attributed the task to the script which
  // scheduled it, keyed by AdScriptIdentifier; that monitor lived in
  // core/ad_tracker/ and walked the V8 stack. It is still taken so callers keep
  // recording which context the task runs in. The companion `enabled` argument
  // switched that attribution off and is gone -- no caller ever passed it.
  AsyncTask(ExecutionContext* execution_context,
            AsyncTaskContext* async_context,
            const char* step = nullptr);
  ~AsyncTask();
};

// Called from generated instrumentation code.
inline CoreProbeSink* ToCoreProbeSink(LocalFrame* frame) {
  return frame ? frame->GetProbeSink() : nullptr;
}

inline CoreProbeSink* ToCoreProbeSink(ExecutionContext* context) {
  return context ? context->GetProbeSink() : nullptr;
}

inline CoreProbeSink* ToCoreProbeSink(Document& document) {
  return ToCoreProbeSink(document.GetExecutionContext());
}

inline CoreProbeSink* ToCoreProbeSink(Document* document) {
  return document ? ToCoreProbeSink(document->GetExecutionContext()) : nullptr;
}

inline CoreProbeSink* ToCoreProbeSink(CoreProbeSink* sink) {
  return sink;
}

inline CoreProbeSink* ToCoreProbeSink(Node& node) {
  return ToCoreProbeSink(node.GetDocument());
}

inline CoreProbeSink* ToCoreProbeSink(Node* node) {
  return node ? ToCoreProbeSink(node->GetDocument()) : nullptr;
}

inline CoreProbeSink* ToCoreProbeSink(EventTarget* event_target) {
  return event_target ? ToCoreProbeSink(event_target->GetExecutionContext())
                      : nullptr;
}

// ToCoreProbeSink(OffscreenCanvas*) is gone along with the
// DidCreateOffscreenCanvasContext probe it served -- see core_probes.pidl.

CORE_EXPORT void AllAsyncTasksCanceled(ExecutionContext*);

}  // namespace probe
}  // namespace blink

#include "base/time/time.h"
#include "third_party/blink/renderer/core/core_probes_inl.h"

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PROBE_CORE_PROBES_H_
