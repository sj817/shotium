// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_SCHEDULER_TASK_ATTRIBUTION_TASK_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_SCHEDULER_TASK_ATTRIBUTION_TASK_STATE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink::scheduler {
class TaskAttributionInfo;
}  // namespace blink::scheduler

namespace blink {
class SchedulerTaskContext;
class ResourceTimingContext;
class SoftNavigationContext;
class ScriptToolContext;

// `TaskAttributionTaskState` objects record which task a piece of work was
// scheduled from. They were never exposed to JS.
//
// Instances of this class will either be WebSchedulingTaskState, if propagating
// `SchedulerTaskContext` (web scheduling APIs), or TaskAttributionInfoImpl,
// which is exposed as TaskAttributionInfo via TaskAttributionTracker public
// APIs.
//
// V8 propagates these objects to continuations by binding the current CPED and
// restoring it in microtasks:
//   1. For promises, the current CPED is bound to the promise reaction at
//      creation time (e.g. when .then() is called or a promise is awaited)
//
//   2. For promises resolved with a custom thennable, there's an extra hop
//      through a microtask to run the custom .then() function. For the promise
//      being resolved, (1) above applies. For the custom .then() function, the
//      resolve-time CPED is bound to the microtask, i.e. the CPED inside the
//      custom .then() function is the same as when the resolve happens, keeping
//      it consistent across the async hop.
//
//   3. For non-promise microtasks, which are used throughout Blink, the current
//      CPED is bound when EnqueueMicrotask() is called.
//
// Similarly, in Blink these objects are propagated to descendant tasks by
// capturing the current CPED during various API calls and restoring it prior to
// running a callback. For example, the current CPED is captured when setTimeout
// is called and restored before running the associated callback.
class CORE_EXPORT TaskAttributionTaskState
    : public GarbageCollected<TaskAttributionTaskState> {
 public:
  // The task state of the task currently running on this thread.
  //
  // This used to live in V8's "continuation preserved embedder data", so that
  // V8 would carry it across promise reactions and microtasks automatically.
  // With no promises and no microtask queue of V8's, propagation across
  // continuations no longer exists and no longer needs to: what remains is the
  // state of the task Blink is running right now, set and cleared by
  // `TaskAttributionTracker::TaskScope`, held in a per-thread pointer.
  static TaskAttributionTaskState* GetCurrent();
  static void SetCurrent(TaskAttributionTaskState*);

  virtual scheduler::TaskAttributionInfo* GetTaskAttributionInfo() = 0;
  virtual SchedulerTaskContext* GetSchedulerTaskContext() = 0;

  // Fork to a new copy, overriding with specified TaskAttributionId and
  // ResourceTimingContext.
  virtual TaskAttributionTaskState* ForkAndSetVariable(
      ResourceTimingContext*) = 0;

  // Fork to a new copy, overriding with specified TaskAttributionId and
  // SoftNavigationContext.
  virtual TaskAttributionTaskState* ForkAndSetVariable(
      SoftNavigationContext*) = 0;

  virtual TaskAttributionTaskState* ForkAndSetVariable(ScriptToolContext*) = 0;

  virtual bool IsWebSchedulingTaskState() const { return false; }
  virtual bool IsTaskAttributionInfoImpl() const { return false; }

  virtual void Trace(Visitor*) const {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_SCHEDULER_TASK_ATTRIBUTION_TASK_STATE_H_
