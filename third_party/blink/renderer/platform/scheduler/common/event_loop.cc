// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/public/event_loop.h"

#include <utility>

#include "base/check.h"
#include "base/memory/ptr_util.h"
#include "base/trace_event/trace_event.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/scheduler/public/frame_or_worker_scheduler.h"
#include "third_party/blink/renderer/platform/scheduler/public/task_attribution_tracker.h"

namespace blink {
namespace scheduler {

EventLoop::PauseMicrotasksHandle::~PauseMicrotasksHandle() {
  CHECK_GT(loop_->microtasks_pause_count_, 0);
  --loop_->microtasks_pause_count_;
}

EventLoop::EventLoop(Delegate* delegate) : delegate_(delegate) {
  DCHECK(delegate);
}

EventLoop::~EventLoop() {
  DCHECK(schedulers_.empty());
}

void EventLoop::EnqueueMicrotask(base::OnceClosure task) {
  pending_microtasks_.push_back(std::move(task));
}

void EventLoop::EnqueueEndOfMicrotaskCheckpointTask(base::OnceClosure task) {
  end_of_checkpoint_tasks_.push_back(std::move(task));
}

void EventLoop::RunEndOfMicrotaskCheckpointTasks() {
  // 4. "For each environment settings object whose responsible event loop is
  // this event loop, notify about rejected promises on that environment
  // settings object." That call was delegate_->NotifyRejectedPromises(); see
  // Agent.

  // 5. Cleanup Indexed Database Transactions.
  if (!end_of_checkpoint_tasks_.empty()) {
    Vector<base::OnceClosure> tasks = std::move(end_of_checkpoint_tasks_);
    for (auto& task : tasks)
      std::move(task).Run();
  }
}

void EventLoop::PerformMicrotaskCheckpoint() {
  if (AreMicrotasksPaused() || ScriptForbiddenScope::IsScriptForbidden()) {
    return;
  }

  if (performing_microtask_checkpoint_) {
    // Re-entrant checkpoints are a no-op: the outer loop below will pick up
    // anything queued in the meantime. This is what V8's microtask queue
    // re-entrancy guard did.
    return;
  }
  base::AutoReset<bool> reset(&performing_microtask_checkpoint_, true);

  // <spec href="https://html.spec.whatwg.org/C#perform-a-microtask-checkpoint">
  // While the microtask queue is not empty: dequeue a microtask and run it.
  // Microtasks queued while running a microtask are appended to the same
  // queue and run in the same checkpoint.
  while (!pending_microtasks_.empty()) {
    TRACE_EVENT0("renderer.scheduler", "RunPendingMicrotask");
    base::OnceClosure task = std::move(pending_microtasks_.front());
    pending_microtasks_.pop_front();
    TaskAttributionTracker::MicrotaskTraceScope scope;
    std::move(task).Run();
  }

  RunEndOfMicrotaskCheckpointTasks();
}

void EventLoop::AttachScheduler(FrameOrWorkerScheduler* scheduler) {
  DCHECK(loop_enabled_);
  DCHECK(!schedulers_.Contains(scheduler));
  schedulers_.insert(scheduler);
}

void EventLoop::DetachScheduler(FrameOrWorkerScheduler* scheduler) {
  DCHECK(loop_enabled_);
  DCHECK(schedulers_.Contains(scheduler));
  schedulers_.erase(scheduler);
}

bool EventLoop::IsSchedulerAttachedForTest(FrameOrWorkerScheduler* scheduler) {
  return schedulers_.Contains(scheduler);
}

std::unique_ptr<EventLoop::PauseMicrotasksHandle> EventLoop::PauseMicrotasks() {
  return base::WrapUnique(new PauseMicrotasksHandle(this));
}

}  // namespace scheduler
}  // namespace blink
