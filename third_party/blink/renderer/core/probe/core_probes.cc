/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/probe/core_probes.h"

#include "base/trace_event/typed_macros.h"
#include "third_party/blink/renderer/core/core_probes_inl.h"
#include "third_party/blink/renderer/core/probe/async_task_context.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {
namespace probe {

base::TimeTicks ProbeBase::CaptureStartTime() const {
  if (start_time_.is_null())
    start_time_ = base::TimeTicks::Now();
  return start_time_;
}

base::TimeTicks ProbeBase::CaptureEndTime() const {
  if (end_time_.is_null())
    end_time_ = base::TimeTicks::Now();
  return end_time_;
}

base::TimeDelta ProbeBase::Duration() const {
  DCHECK(!start_time_.is_null());
  return CaptureEndTime() - start_time_;
}

AsyncTask::AsyncTask(ExecutionContext* context,
                     AsyncTaskContext* task_context,
                     const char* step) {
  // TODO(crbug.com/1275875): Verify that `task_context` was scheduled, but
  // not yet canceled. Currently we don't have enough confidence that such
  // a CHECK wouldn't break blink.

  // ScriptInitiationMonitor::DidStartAsyncTask()/DidFinishAsyncTask() bracketed
  // this scope so the AdTracker could tell, while the task ran, which script had
  // scheduled it. The monitor lived in core/ad_tracker/ and answered that by
  // walking the V8 stack. The perfetto flow below is blink's own tracing and is
  // the whole of what this class does now, so `task_context` is no longer
  // retained past the constructor.
  //
  // `step` used to be stashed in a recurring_ field and read back in the
  // destructor to tell the V8 inspector debugger whether a non-recurring
  // task had finished (AsyncTaskCanceled) or a recurring one had merely
  // paused (AsyncTaskFinished). The debugger is gone with V8, so nothing
  // reads that distinction anymore and `step` itself now goes unused.
  TRACE_EVENT_BEGIN("blink", "AsyncTask Run",
                    perfetto::Flow::FromPointer(task_context));
}

AsyncTask::~AsyncTask() {
  TRACE_EVENT_END("blink");  // "AsyncTask Run"
}

void AllAsyncTasksCanceled(ExecutionContext* context) {}

}  // namespace probe
}  // namespace blink
