// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/worker/non_main_thread_task_queue.h"

#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/task/sequence_manager/sequence_manager.h"
#include "base/task/single_thread_task_runner.h"
#include "third_party/blink/renderer/platform/scheduler/common/blink_scheduler_single_thread_task_runner.h"
#include "third_party/blink/renderer/platform/scheduler/common/task_priority.h"
#include "third_party/blink/renderer/platform/scheduler/worker/non_main_thread_scheduler_base.h"

namespace blink {
namespace scheduler {

using base::sequence_manager::TaskQueue;

NonMainThreadTaskQueue::NonMainThreadTaskQueue(
    base::sequence_manager::SequenceManager& sequence_manager,
    const TaskQueue::Spec& spec,
    NonMainThreadSchedulerBase* non_main_thread_scheduler,
    scoped_refptr<base::SingleThreadTaskRunner> thread_task_runner)
    : task_queue_(sequence_manager.CreateTaskQueue(spec)),
      non_main_thread_scheduler_(non_main_thread_scheduler),
      thread_task_runner_(std::move(thread_task_runner)),
      task_runner_with_default_task_type_(
          WrapTaskRunner(task_queue_->task_runner())) {
  if (spec.should_notify_observers) {
    // TaskQueueImpl may be null for tests.
    task_queue_->SetOnTaskCompletedHandler(base::BindRepeating(
        &NonMainThreadTaskQueue::OnTaskCompleted, base::Unretained(this)));
  }
}

NonMainThreadTaskQueue::~NonMainThreadTaskQueue() = default;

void NonMainThreadTaskQueue::ShutdownTaskQueue() {
  non_main_thread_scheduler_ = nullptr;
  task_queue_.reset();
}

void NonMainThreadTaskQueue::OnTaskCompleted(
    const base::sequence_manager::Task& task,
    TaskQueue::TaskTiming* task_timing,
    base::LazyNow* lazy_now) {
  // |non_main_thread_scheduler_| can be nullptr in tests.
  if (non_main_thread_scheduler_) {
    // The last ref to `non_main_thread_scheduler_` might be released as part of
    // this task's cleanup microtasks, make sure it lives through its own
    // cleanup: crbug.com/1464113.
    auto self_ref = WrapRefCounted(this);
    non_main_thread_scheduler_->OnTaskCompleted(this, task, task_timing,
                                                lazy_now);
  }
}

scoped_refptr<base::SingleThreadTaskRunner>
NonMainThreadTaskQueue::CreateTaskRunner(TaskType task_type) {
  return WrapTaskRunner(
      task_queue_->CreateTaskRunner(static_cast<int>(task_type)));
}

scoped_refptr<BlinkSchedulerSingleThreadTaskRunner>
NonMainThreadTaskQueue::WrapTaskRunner(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner) {
  // `thread_task_runner_` can be null if the default task runner wasn't set up
  // prior to creating this task queue. That's okay because the lifetime of
  // task queues created early matches the thead scheduler.
  return base::MakeRefCounted<BlinkSchedulerSingleThreadTaskRunner>(
      std::move(task_runner), thread_task_runner_);
}

}  // namespace scheduler
}  // namespace blink
