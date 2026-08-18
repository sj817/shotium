// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PROBE_ASYNC_TASK_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PROBE_ASYNC_TASK_CONTEXT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace blink {
class ExecutionContext;
namespace probe {

// Tracks scheduling and cancelation of a single async task.
// An async task scheduled via `AsyncTaskContext` is guaranteed to be
// canceled.
class CORE_EXPORT AsyncTaskContext {
 public:
  AsyncTaskContext() = default;
  ~AsyncTaskContext();

  enum class StackOptions {
    kDoNotScan,
    kScan,
  };

  // Not copyable or movable. The address of `AsyncTaskContext` is used
  // to identify this task and corresponding runs/invocations via `AsyncTask`.
  AsyncTaskContext(const AsyncTaskContext&) = delete;
  AsyncTaskContext& operator=(const AsyncTaskContext&) = delete;

  // Schedules this async task. `Schedule` can be called
  // once and only once per AsyncTaskContext instance. `stack_options` marked
  // the cases where blink runs an internal operation asynchronously and
  // something on the other side asks, while the task runs, whether ad script
  // scheduled it. That question was answered by walking the V8 stack, so
  // `kScan` no longer does anything -- see the note in Schedule().
  void Schedule(ExecutionContext* context,
                const StringView& name,
                StackOptions stack_options = StackOptions::kDoNotScan);

  // Explicitly cancel this async task. No `AsyncTasks`s must be created with
  // this context after `Cancel` was called.
  void Cancel();

  // SetAdTask()/IsAdTask()/ad_identifier() were here. They recorded which ad
  // script scheduled this task, keyed by AdScriptIdentifier -- a v8::Context
  // debugger id plus a v8 script id. Neither exists without V8, and the only
  // caller was AdTracker, which scanned the JavaScript stack.

  // The Id uniquely identifies this task. It is calculated based on the
  // address of `AsyncTaskContext`.
  void* Id() const;

 private:
  friend class AsyncTask;
};

}  // namespace probe
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PROBE_ASYNC_TASK_CONTEXT_H_
