// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_EVENT_LOOP_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_EVENT_LOOP_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/auto_reset.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/deque.h"
#include "third_party/blink/renderer/platform/wtf/hash_set.h"
#include "third_party/blink/renderer/platform/wtf/ref_counted.h"

namespace blink {

class Agent;
class FrameOrWorkerScheduler;

namespace scheduler {

// Represents an event loop. The instance is held by ExecutionContexts.
// https://html.spec.whatwg.org/C#event-loop
//
// Browsing contexts must share the same EventLoop if they have a chance to
// access each other synchronously.
// That is:
//  - Two Documents must share the same EventLoop if they are scriptable with
//    each other.
//  - Workers and Worklets can have its own EventLoop, as no other browsing
//    context can access it synchronously.
//
// The specification says an event loop has (non-micro) task queues. However,
// we process regular tasks in a different granularity; in our implementation,
// a frame has task queues. This is an intentional violation of the
// specification.
//
// Therefore, currently, EventLoop is a unit that just manages a microtask
// queue: <https://html.spec.whatwg.org/C#microtask-queue>
//
// Microtasks queued during a task are executed at the end of the task or
// after a user script is executed (for the exact timings, refer to the
// specification). Some web platform features require this functionality.
//
// Implementation notes: microtask queues originally lived in V8, for
// JavaScript promises, and this class enqueued Blink's own microtasks into a
// per-EventLoop V8 microtask queue so both kinds ran interleaved in queue
// order. There are no JavaScript microtasks left, so the queue below is
// Blink's own: EnqueueMicrotask() appends to it and PerformMicrotaskCheckpoint()
// drains it, including anything queued while draining, per the spec's
// "perform a microtask checkpoint" algorithm.
class PLATFORM_EXPORT EventLoop final : public RefCounted<EventLoop> {
  USING_FAST_MALLOC(EventLoop);

 public:
  // A pure virtual class implemented by the `environment settings object`.
  // Callbacks exist for steps completed in the microtask completion algorithm.
  // Its one callback, NotifyRejectedPromises(), is gone with RejectedPromises;
  // the interface and the delegate pointer stay because the microtask
  // checkpoint is the natural place for the next such step and removing them
  // would change EventLoop's constructor for every caller.
  class Delegate : public GarbageCollectedMixin {};

  EventLoop(const EventLoop&) = delete;
  EventLoop& operator=(const EventLoop&) = delete;

  // Queues |cb| onto this event loop's microtask queue.
  void EnqueueMicrotask(base::OnceClosure cb);

  // Runs |cb| at the end of microtask checkpoint.
  // The tasks are run when control is returning to C++ from script, after
  // executing a script task (e.g. callback, event) or microtasks
  // (e.g. promise). This is explicitly needed for Indexed DB transactions
  // per spec, but should in general be avoided.
  void EnqueueEndOfMicrotaskCheckpointTask(base::OnceClosure cb);

  // Run any pending tasks.
  void RunEndOfMicrotaskCheckpointTasks();

  // Runs pending microtasks until the queue is empty, then runs the
  // end-of-checkpoint tasks.
  void PerformMicrotaskCheckpoint();

  void AttachScheduler(FrameOrWorkerScheduler*);
  void DetachScheduler(FrameOrWorkerScheduler*);

  bool IsSchedulerAttachedForTest(FrameOrWorkerScheduler*);

  class PLATFORM_EXPORT PauseMicrotasksHandle {
   public:
    ~PauseMicrotasksHandle();
    PauseMicrotasksHandle(const PauseMicrotasksHandle& r) = delete;
    PauseMicrotasksHandle& operator=(const PauseMicrotasksHandle& r) = delete;

   private:
    friend class EventLoop;
    explicit PauseMicrotasksHandle(scoped_refptr<EventLoop> loop)
        : loop_(std::move(loop)) {
      ++loop_->microtasks_pause_count_;
    }
    scoped_refptr<EventLoop> loop_;
  };

  // Suppresses microtask execution for the lifetime of the returned handle.
  // Pending microtasks would be executed as soon as all issued handles go
  // out of scope.
  [[nodiscard]] std::unique_ptr<PauseMicrotasksHandle> PauseMicrotasks();
  bool AreMicrotasksPaused() const { return !!microtasks_pause_count_; }

 private:
  friend class RefCounted<EventLoop>;
  friend blink::Agent;

  explicit EventLoop(Delegate* delegate);
  ~EventLoop();

  WeakPersistent<Delegate> delegate_;
  int microtasks_pause_count_ = 0;
  bool loop_enabled_ = true;
  bool performing_microtask_checkpoint_ = false;
  Deque<base::OnceClosure> pending_microtasks_;
  Vector<base::OnceClosure> end_of_checkpoint_tasks_;
  HashSet<FrameOrWorkerScheduler*> schedulers_;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_EVENT_LOOP_H_
