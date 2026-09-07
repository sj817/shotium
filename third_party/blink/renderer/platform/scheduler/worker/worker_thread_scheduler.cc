// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/worker/worker_thread_scheduler.h"

#include <memory>

#include "base/task/sequence_manager/sequence_manager.h"
#include "base/task/sequence_manager/task_queue.h"
#include "base/task/single_thread_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "third_party/blink/renderer/platform/scheduler/worker/non_main_thread_scheduler_helper.h"
#include "third_party/perfetto/include/perfetto/tracing/track_event_args.h"

namespace blink {
namespace scheduler {

using base::sequence_manager::TaskQueue;

WorkerThreadScheduler::WorkerThreadScheduler(
    base::sequence_manager::SequenceManager* sequence_manager)
    : NonMainThreadSchedulerBase(sequence_manager,
                                 TaskType::kWorkerThreadTaskQueueDefault),
      idle_helper_queue_(GetHelper().NewTaskQueue(
          TaskQueue::Spec(base::sequence_manager::QueueName::WORKER_IDLE_TQ))),
      idle_helper_(&GetHelper(),
                   this,
                   "WorkerSchedulerIdlePeriod",
                   base::Milliseconds(300),
                   idle_helper_queue_->GetTaskQueue()) {

  GetHelper().SetObserver(this);

  TRACE_EVENT_INSTANT(
      TRACE_DISABLED_BY_DEFAULT("worker.scheduler"), "WorkerScheduler:created",
      perfetto::Flow::FromPointer(this, "WorkerThreadScheduler"));
}

WorkerThreadScheduler::~WorkerThreadScheduler() {
  TRACE_EVENT_INSTANT(
      TRACE_DISABLED_BY_DEFAULT("worker.scheduler"), "WorkerScheduler:deleted",
      perfetto::TerminatingFlow::FromPointer(this, "WorkerThreadScheduler"));
}

scoped_refptr<SingleThreadIdleTaskRunner>
WorkerThreadScheduler::IdleTaskRunner() {
  DCHECK(initialized_);
  return idle_helper_.IdleTaskRunner();
}

scoped_refptr<base::SingleThreadTaskRunner>
WorkerThreadScheduler::CleanupTaskRunner() {
  return DefaultTaskQueue()->GetTaskRunnerWithDefaultTaskType();
}

bool WorkerThreadScheduler::ShouldYieldForHighPriorityWork() {
  // We don't consider any work as being high priority on workers.
  return false;
}

void WorkerThreadScheduler::AddTaskObserver(base::TaskObserver* task_observer) {
  DCHECK(initialized_);
  GetHelper().AddTaskObserver(task_observer);
}

void WorkerThreadScheduler::RemoveTaskObserver(
    base::TaskObserver* task_observer) {
  DCHECK(initialized_);
  GetHelper().RemoveTaskObserver(task_observer);
}

void WorkerThreadScheduler::Shutdown() {
  DCHECK(initialized_);
  ThreadSchedulerBase::Shutdown();
  idle_helper_.Shutdown();
  idle_helper_queue_->ShutdownTaskQueue();
  GetHelper().Shutdown();
}

scoped_refptr<NonMainThreadTaskQueue>
WorkerThreadScheduler::DefaultTaskQueue() {
  DCHECK(initialized_);
  return GetHelper().DefaultNonMainThreadTaskQueue();
}

void WorkerThreadScheduler::Init() {
  initialized_ = true;
  idle_helper_.EnableLongIdlePeriod();
}

void WorkerThreadScheduler::OnTaskCompleted(
    NonMainThreadTaskQueue* task_queue,
    const base::sequence_manager::Task& task,
    TaskQueue::TaskTiming* task_timing,
    base::LazyNow* lazy_now) {
  task_timing->RecordTaskEnd(lazy_now);
  DispatchOnTaskCompletionCallbacks();
}

SchedulerHelper* WorkerThreadScheduler::GetSchedulerHelperForTesting() {
  return &GetHelper();
}

bool WorkerThreadScheduler::CanEnterLongIdlePeriod(base::TimeTicks,
                                                   base::TimeDelta*) {
  return true;
}

base::TimeTicks WorkerThreadScheduler::CurrentIdleTaskDeadlineForTesting()
    const {
  return idle_helper_.CurrentIdleTaskDeadlineForTesting();
}

scoped_refptr<NonMainThreadTaskQueue>
WorkerThreadScheduler::ControlTaskQueue() {
  return GetHelper().ControlNonMainThreadTaskQueue();
}

base::SequencedTaskRunner* WorkerThreadScheduler::GetVirtualTimeTaskRunner() {
  // Note this is not Control task runner because it has task notifications
  // disabled.
  return DefaultTaskQueue()->GetTaskRunnerWithDefaultTaskType().get();
}

void WorkerThreadScheduler::PostIdleTask(const base::Location& location,
                                         Thread::IdleTask task) {
  IdleTaskRunner()->PostIdleTask(location, std::move(task));
}

void WorkerThreadScheduler::RemoveCancelledIdleTasks() {
  idle_helper_.RemoveCancelledIdleTasks();
}

void WorkerThreadScheduler::PostDelayedIdleTask(const base::Location& location,
                                                base::TimeDelta delay,
                                                Thread::IdleTask task) {
  IdleTaskRunner()->PostDelayedIdleTask(location, delay, std::move(task));
}

base::TimeTicks WorkerThreadScheduler::MonotonicallyIncreasingVirtualTime() {
  return base::TimeTicks::Now();
}

}  // namespace scheduler
}  // namespace blink
