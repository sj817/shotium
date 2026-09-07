// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_WORKER_THREAD_SCHEDULER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_WORKER_THREAD_SCHEDULER_H_

#include "base/task/single_thread_task_runner.h"
#include "third_party/blink/renderer/platform/scheduler/common/idle_helper.h"
#include "third_party/blink/renderer/platform/scheduler/worker/non_main_thread_scheduler_base.h"

namespace base {
class LazyNow;
class TaskObserver;
namespace sequence_manager {
class SequenceManager;
}
}  // namespace base

namespace blink {
namespace scheduler {


class PLATFORM_EXPORT WorkerThreadScheduler : public NonMainThreadSchedulerBase,
                                              public ThreadScheduler,
                                              public IdleHelper::Delegate {
 public:
  // Background scheduling for font decoding and HTML preloading.
  // |sequence_manager| must remain valid for this object's lifetime.
  explicit WorkerThreadScheduler(
      base::sequence_manager::SequenceManager* sequence_manager);
  WorkerThreadScheduler(const WorkerThreadScheduler&) = delete;
  WorkerThreadScheduler& operator=(const WorkerThreadScheduler&) = delete;
  ~WorkerThreadScheduler() override;

  // ThreadScheduler implementation:
  scoped_refptr<base::SingleThreadTaskRunner> CleanupTaskRunner() override;
  bool ShouldYieldForHighPriorityWork() override;
  void AddTaskObserver(base::TaskObserver* task_observer) override;
  void RemoveTaskObserver(base::TaskObserver* task_observer) override;
  void PostIdleTask(const base::Location&, Thread::IdleTask) override;
  void PostDelayedIdleTask(const base::Location&,
                           base::TimeDelta delay,
                           Thread::IdleTask) override;
  void RemoveCancelledIdleTasks() override;
  base::TimeTicks MonotonicallyIncreasingVirtualTime() override;
  void Shutdown() override;

  // NonMainThreadSchedulerImpl implementation:
  void Init() override;
  scoped_refptr<NonMainThreadTaskQueue> DefaultTaskQueue() override;
  void OnTaskCompleted(
      NonMainThreadTaskQueue* worker_task_queue,
      const base::sequence_manager::Task& task,
      base::sequence_manager::TaskQueue::TaskTiming* task_timing,
      base::LazyNow* lazy_now) override;

  SchedulerHelper* GetSchedulerHelperForTesting();
  base::TimeTicks CurrentIdleTaskDeadlineForTesting() const;

  scoped_refptr<SingleThreadIdleTaskRunner> IdleTaskRunner();

  // Returns the control task queue.  Tasks posted to this queue are executed
  // with the highest priority. Care must be taken to avoid starvation of other
  // task queues.
  scoped_refptr<NonMainThreadTaskQueue> ControlTaskQueue();

 protected:
  // IdleHelper::Delegate implementation:
  bool CanEnterLongIdlePeriod(
      base::TimeTicks now,
      base::TimeDelta* next_long_idle_period_delay_out) override;
  void IsNotQuiescent() override {}
  void OnPendingTasksChanged(bool new_state) override {}

 private:
  // ThreadSchedulerBase overrides
  base::SequencedTaskRunner* GetVirtualTimeTaskRunner() override;

  scoped_refptr<NonMainThreadTaskQueue> idle_helper_queue_;
  IdleHelper idle_helper_;
  bool initialized_ = false;
};

}  // namespace scheduler
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_WORKER_WORKER_THREAD_SCHEDULER_H_
