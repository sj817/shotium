// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/public/thread.h"

#include "base/task/single_thread_task_runner.h"
#include "build/build_config.h"
#include "third_party/blink/renderer/platform/scheduler/public/main_thread.h"
#include "third_party/blink/renderer/platform/scheduler/public/thread_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/std_lib_extras.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#elif BUILDFLAG(IS_POSIX) || BUILDFLAG(IS_FUCHSIA)
#include <unistd.h>
#endif

namespace blink {

namespace {

constinit thread_local Thread* current_thread = nullptr;

std::unique_ptr<MainThread>& GetMainThread() {
  DEFINE_STATIC_LOCAL(std::unique_ptr<MainThread>, main_thread, ());
  return main_thread;
}

}  // namespace

// static
void Thread::UpdateThreadTLS(Thread* thread) {
  current_thread = thread;
}

ThreadCreationParams::ThreadCreationParams(ThreadType thread_type)
    : thread_type(thread_type),
      name(GetNameForThreadType(thread_type)),
      supports_gc(false) {}

ThreadCreationParams& ThreadCreationParams::SetThreadNameForTest(
    const char* thread_name) {
  name = thread_name;
  return *this;
}

ThreadCreationParams& ThreadCreationParams::SetSupportsGC(bool gc_enabled) {
  supports_gc = gc_enabled;
  return *this;
}

Thread* Thread::Current() {
  return current_thread;
}

MainThread* Thread::MainThread() {
  return GetMainThread().get();
}

std::unique_ptr<MainThread> MainThread::SetMainThread(
    std::unique_ptr<MainThread> main_thread) {
  current_thread = main_thread.get();
  std::swap(GetMainThread(), main_thread);
  return main_thread;
}

Thread::Thread() = default;

Thread::~Thread() = default;

bool Thread::IsCurrentThread() const {
  return current_thread == this;
}

void Thread::AddTaskObserver(TaskObserver* task_observer) {
  CHECK(IsCurrentThread());
  Scheduler()->AddTaskObserver(task_observer);
}

void Thread::RemoveTaskObserver(TaskObserver* task_observer) {
  CHECK(IsCurrentThread());
  Scheduler()->RemoveTaskObserver(task_observer);
}

#if BUILDFLAG(IS_WIN)
static_assert(sizeof(blink::PlatformThreadId) >= sizeof(DWORD),
              "size of platform thread id is too small");
#elif BUILDFLAG(IS_POSIX) || BUILDFLAG(IS_FUCHSIA)
static_assert(sizeof(blink::PlatformThreadId) >= sizeof(pid_t),
              "size of platform thread id is too small");
#else
#error Unexpected platform
#endif

}  // namespace blink
