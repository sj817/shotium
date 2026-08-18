// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/thread_state.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/bindings/active_script_wrappable_manager.h"
#include "third_party/blink/renderer/platform/heap/custom_spaces.h"
#include "third_party/blink/renderer/platform/heap/thread_state_storage.h"
#include "cppgc/default-platform.h"
#include "cppgc/heap-state.h"
#include "cppgc/platform.h"
#include "v8/include/cppgc/heap-consistency.h"

namespace blink {

namespace {

// One platform for the process, shared by every thread's heap.
//
// cppgc::DefaultPlatform is V8's own implementation, built on libplatform,
// which is vendored alongside cppgc. It is a real implementation -- it
// allocates pages through v8::base::PageAllocator and runs jobs on its own
// worker threads.
//
// It is not, however, integrated with Chromium's task scheduler the way
// gin::V8Platform was; it brings its own thread pool. That is a deliberate
// trade for now. The heaps below are configured for atomic marking and
// sweeping, so those workers stay idle: GC happens in a stop-the-world pause
// on the owning thread. For a process that renders one page and exits, the
// jank that incremental and concurrent GC exist to avoid is not a concern, and
// a single-threaded collector is one less source of nondeterminism while the
// rest of the pipeline is being brought up.
std::shared_ptr<cppgc::Platform>& ProcessPlatform() {
  static base::NoDestructor<std::shared_ptr<cppgc::Platform>> platform([] {
    auto default_platform = std::make_shared<cppgc::DefaultPlatform>();
    // Standalone cppgc requires this before anything else it owns is used. It
    // builds the process-wide GCInfoTable -- the table every
    // MakeGarbageCollected<T> looks T up in to get its trace and finalize
    // callbacks -- out of the page allocator it is handed here.
    //
    // With V8 in the process this happened inside v8::V8::InitializePlatform().
    // There is no V8 here and nothing took the job over, so the table stayed
    // null and the very first allocation in blink startup locked a mutex
    // through a null pointer.
    cppgc::InitializeProcess(default_platform->GetPageAllocator());
    return std::shared_ptr<cppgc::Platform>(std::move(default_platform));
  }());
  return *platform;
}

cppgc::Heap::HeapOptions HeapOptions(
    std::optional<cppgc::StackStartMarker> stack_start_marker) {
  cppgc::Heap::HeapOptions options;
  options.custom_spaces = CustomSpaces::CreateCustomSpaces();
  options.marking_support = cppgc::Heap::MarkingType::kAtomic;
  options.sweeping_support = cppgc::Heap::SweepingType::kAtomic;
  options.stack_start_marker = std::move(stack_start_marker);
  return options;
}

}  // namespace

// static
ThreadState* ThreadState::AttachMainThread(
    std::optional<cppgc::StackStartMarker> stack_start_marker) {
  auto* thread_state =
      new ThreadState(ProcessPlatform(), std::move(stack_start_marker));
  ThreadStateStorage::AttachMainThread(
      *thread_state, thread_state->heap().GetAllocationHandle(),
      thread_state->heap().GetHeapHandle());
  thread_state->CreateActiveScriptWrappableManager();
  return thread_state;
}

// static
ThreadState* ThreadState::AttachMainThreadForTesting(
    cppgc::Platform* platform) {
  // The caller owns the platform in this path, so the shared_ptr must not
  // delete it.
  auto* thread_state = new ThreadState(
      std::shared_ptr<cppgc::Platform>(platform, [](cppgc::Platform*) {}));
  ThreadStateStorage::AttachMainThread(
      *thread_state, thread_state->heap().GetAllocationHandle(),
      thread_state->heap().GetHeapHandle());
  thread_state->CreateActiveScriptWrappableManager();
  return thread_state;
}

// static
ThreadState* ThreadState::AttachCurrentThread() {
  auto* thread_state = new ThreadState(ProcessPlatform());
  ThreadStateStorage::AttachNonMainThread(
      *thread_state, thread_state->heap().GetAllocationHandle(),
      thread_state->heap().GetHeapHandle());
  thread_state->CreateActiveScriptWrappableManager();
  return thread_state;
}

// static
ThreadState* ThreadState::AttachCurrentThreadForTesting(
    cppgc::Platform* platform) {
  auto* thread_state = new ThreadState(
      std::shared_ptr<cppgc::Platform>(platform, [](cppgc::Platform*) {}));
  ThreadStateStorage::AttachNonMainThread(
      *thread_state, thread_state->heap().GetAllocationHandle(),
      thread_state->heap().GetHeapHandle());
  thread_state->CreateActiveScriptWrappableManager();
  return thread_state;
}

// static
void ThreadState::DetachCurrentThread() {
  auto* state = ThreadState::Current();
  DCHECK(state);
  delete state;
}

ThreadState::ThreadState(
    std::shared_ptr<cppgc::Platform> platform,
    std::optional<cppgc::StackStartMarker> stack_start_marker)
    : platform_(std::move(platform)),
      heap_(cppgc::Heap::Create(platform_,
                                HeapOptions(std::move(stack_start_marker)))),
      heap_handle_(heap_->GetHeapHandle()),
      thread_id_(CurrentThread()) {}

// Upstream allocated the manager in AttachToIsolate(), because its whole point
// was to be consulted from V8's GC prologue. With a standalone heap there is no
// attach-to-isolate step, and the port moved the allocation into this
// constructor -- which crashed on the first line of blink startup, because
// MakeGarbageCollected<> asks ThreadStateStorage for the allocation handle and
// the storage is only given this ThreadState after the constructor returns.
// The allocation belongs just after that registration instead.
void ThreadState::CreateActiveScriptWrappableManager() {
  DCHECK(!active_script_wrappable_manager_);
  active_script_wrappable_manager_ =
      MakeGarbageCollected<ActiveScriptWrappableManager>();
}

ThreadState::~ThreadState() {
  DCHECK(IsCreationThread());
  heap_.reset();
  ThreadStateStorage::DetachNonMainThread(*ThreadStateStorage::Current());
}

void ThreadState::CollectAllGarbageForTesting(StackState stack_state) {
  // Collect repeatedly: finalizers registered during one collection can drop
  // the last reference to objects that only become collectable in the next.
  // The unified-heap version of this loop stopped early once the live byte
  // count stopped moving, which it read through v8::CppHeap::CollectStatistics;
  // standalone cppgc does not expose that outside its internals, so the loop
  // just runs its bound.
  for (size_t i = 0; i < 5; i++) {
    heap_->ForceGarbageCollectionSlow("ThreadState",
                                      "CollectAllGarbageForTesting",
                                      stack_state);
  }
}

void ThreadState::CollectAllGarbageForMemoryPressure() {
  CollectAllGarbageForTesting(StackState::kMayContainHeapPointers);
}

bool ThreadState::IsIncrementalMarking() const {
  return cppgc::subtle::HeapState::IsMarking(heap_handle()) &&
         !cppgc::subtle::HeapState::IsInAtomicPause(heap_handle());
}

bool ThreadState::IsSweepingOnOwningThread() const {
  return cppgc::subtle::HeapState::IsSweepingOnOwningThread(heap_handle());
}

}  // namespace blink
