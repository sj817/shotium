// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_THREAD_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_THREAD_STATE_H_

#include <memory>

#include "base/compiler_specific.h"
#include "base/functional/callback_forward.h"
#include "build/build_config.h"
#include "third_party/blink/renderer/platform/bindings/active_script_wrappable_manager.h"
#include "third_party/blink/renderer/platform/heap/forward.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/heap/thread_state_storage.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"
#include "third_party/blink/renderer/platform/wtf/threading.h"
#include "cppgc/common.h"  // IWYU pragma: export (for ThreadState::StackState alias)
#include "cppgc/heap.h"

namespace cppgc {
class Platform;
class StackStartMarker;
}  // namespace cppgc

namespace blink {

class ActiveScriptWrappableManager;
class BlinkGCMemoryDumpProvider;

// ThreadState owns the Oilpan heap for a thread.
//
// This used to manage garbage collection *together with V8*: the heap was a
// v8::CppHeap owned by a v8::Isolate, and unified-heap collections were driven
// from V8's GC prologue and epilogue callbacks. There is no V8 here any more,
// so the heap is a standalone cppgc::Heap that this class owns outright, and
// collections are driven by cppgc itself.
//
// What went away with the Isolate, and why it was not replaced:
//
//   AttachToIsolate/DetachFromIsolate, ReleaseCppHeap, GcPrologue/GcEpilogue,
//   BlinkRootsHandler   -- all of these existed to keep V8's wrapper objects
//                          and Blink's C++ objects alive as one graph. With no
//                          wrappers, there is one graph.
//   The heap snapshot entry points -- they went through v8::HeapProfiler.
class PLATFORM_EXPORT ThreadState final {
 public:
  class GCForbiddenScope;
  class NoAllocationScope;

  using StackState = cppgc::EmbedderStackState;

  ALWAYS_INLINE static ThreadState* Current() {
    return &ThreadStateStorage::Current()->thread_state();
  }

  // Attaches a ThreadState to the main-thread.
  static ThreadState* AttachMainThread(
      std::optional<cppgc::StackStartMarker> stack_start_marker = std::nullopt);
  // Attaches a ThreadState to the currently running thread. Must not be the
  // main thread and must be called after AttachMainThread().
  static ThreadState* AttachCurrentThread();
  static void DetachCurrentThread();

  ALWAYS_INLINE cppgc::HeapHandle& heap_handle() const { return heap_handle_; }
  ALWAYS_INLINE cppgc::Heap& heap() const { return *heap_; }

  bool IsMainThread() const {
    return this ==
           &ThreadStateStorage::MainThreadStateStorage()->thread_state();
  }
  bool IsCreationThread() const { return thread_id_ == CurrentThread(); }

  bool IsAllocationAllowed() const {
    return cppgc::subtle::DisallowGarbageCollectionScope::
        IsGarbageCollectionAllowed(heap_handle_);
  }

  // Returns true if incremental marking is currently running, and false
  // otherwise.
  bool IsIncrementalMarking() const;

  // Returns true if the current thread is currently sweeping, i.e., whether the
  // caller is invoked from a destructor, and false otherwise.
  bool IsSweepingOnOwningThread() const;

  static ThreadState* AttachMainThreadForTesting(cppgc::Platform*);
  static ThreadState* AttachCurrentThreadForTesting(cppgc::Platform*);

  // Forced garbage collection for testing.
  void CollectAllGarbageForTesting(
      StackState stack_state = StackState::kNoHeapPointers);

  // Forced garbage collection under memory pressure. Same work as the testing
  // entry point above, separately named because it has a production caller:
  // OomInterventionImpl, which used to send a critical memory pressure
  // notification to every V8 isolate. cppgc has no pressure notification, so
  // the intervention has to collect directly.
  void CollectAllGarbageForMemoryPressure();

  ActiveScriptWrappableManager* GetActiveScriptWrappableManager() {
    return active_script_wrappable_manager_.Get();
  }

 private:
  explicit ThreadState(
      std::shared_ptr<cppgc::Platform>,
      std::optional<cppgc::StackStartMarker> stack_start_marker = std::nullopt);
  ~ThreadState();

  // Allocates active_script_wrappable_manager_. This cannot happen in the
  // constructor: the manager is garbage-collected, and MakeGarbageCollected<>
  // reaches the allocation handle through ThreadStateStorage, which does not
  // know about this ThreadState until the Attach* function that constructed it
  // registers it. So every Attach* path calls this immediately afterwards.
  void CreateActiveScriptWrappableManager();

  // Held for as long as the heap lives: cppgc::Heap::Create takes a
  // shared_ptr and the heap keeps using the platform for the whole of its
  // lifetime, including during teardown.
  std::shared_ptr<cppgc::Platform> platform_;
  std::unique_ptr<cppgc::Heap> heap_;
  cppgc::HeapHandle& heap_handle_;
  base::PlatformThreadId thread_id_;
  Persistent<ActiveScriptWrappableManager> active_script_wrappable_manager_;

  friend class BlinkGCMemoryDumpProvider;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_HEAP_THREAD_STATE_H_
