// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_RUNTIME_H_
#define SHOT_SHOT_RUNTIME_H_

#include <memory>
#include <string>

#include "base/functional/callback_helpers.h"
#include "base/memory_coordinator/memory_consumer_registry.h"
#include "base/types/expected.h"
#include "shot/shot_network.h"

namespace blink {
namespace scheduler {
class WebThreadScheduler;
}  // namespace scheduler
}  // namespace blink

namespace discardable_memory {
class DiscardableSharedMemoryManager;
}  // namespace discardable_memory

namespace shot {

class ShotPlatform;

// Everything a process needs before blink will render anything, brought up once
// and held for as long as the process lives.
//
// This used to be the body of main(), which was fine while shot rendered one
// document and exited. It is a class now because a resident worker renders many
// documents over one process lifetime: blink is a process-wide singleton --
// Platform::InitializeBlink() builds WTF and the partitions once,
// cppgc::InitializeProcess() sits behind a NoDestructor, and Initialize() binds
// the scheduler to this thread -- so the setup cannot be repeated per render,
// and the teardown cannot happen until the process is done.
//
// Construction order is not negotiable and the comments on each step say why.
// Destruction is the reverse, which is why the members are declared in the
// order they are built rather than grouped by kind: shutting the scheduler down
// before the heap that points into it, and the thread pool last, is a
// requirement of ~MainThreadSchedulerImpl's own CHECK.
//
// Must be created on, and destroyed on, the thread that will do the rendering.
class ShotRuntime {
 public:
  // Brings up ICU, the resource bundle, the thread pool, mojo, blink's
  // scheduler, discardable memory, blink itself, and //net.
  static base::expected<std::unique_ptr<ShotRuntime>, std::string> Create(
      const NetworkConfig& network_config);

  ShotRuntime(const ShotRuntime&) = delete;
  ShotRuntime& operator=(const ShotRuntime&) = delete;
  ~ShotRuntime();

  // Hands back everything this process is holding that it can rebuild.
  //
  // A worker that renders one document and exits does not need this; one that
  // stays resident does, because every cache blink and skia keep is sized for
  // a browser tab that will be asked for the same font and the same image
  // again. Between bursts a shot worker will not be, and holding a warm cache
  // for a queue that is empty is the whole difference between a resident pool
  // that costs what it renders and one that costs what it ever rendered.
  //
  // Must be called on the rendering thread, and not while a capture is in
  // flight: it collects blink's heap, which is only safe between documents.
  void PurgeMemory();

  // Gives the pages themselves back, which PurgeMemory() deliberately does
  // not. Everything it frees is decommitted address space; the working set
  // still holds every page this process has touched since it started, most of
  // them shot.exe's own code faulted in by a path that ran once.
  //
  // Separate because the two have different prices. Purging costs a
  // collection and rebuilds caches that were about to be cold anyway.
  // Trimming costs a soft fault per page on the next request -- measured at
  // about 8 ms for a worker that had settled -- and is worth paying only
  // when nobody is likely to ask again soon, which is a longer silence than
  // the one that makes a purge worthwhile.
  void ReleaseWorkingSet();

 private:
  ShotRuntime();

  // Somewhere for memory consumers to register.
  //
  // base::MemoryConsumerRegistry is abstract, and the only concrete production
  // implementation is content's, which groups consumers and reports them to a
  // MemoryConsumerGroupController -- a browser-process memory coordinator that
  // decides who should shrink when the device is under pressure. shot has no
  // coordinator and no second process to coordinate with, so it supplies the
  // other half of the contract itself: consumers register and unregister, and
  // nothing ever asks them to release.
  //
  // That is the whole behaviour of a registry in a process nobody is
  // coordinating -- not a stub standing in for one. The consumer that needs it
  // here is DiscardableSharedMemoryManager, whose constructor registers so that
  // a coordinator *could* tell it to purge.
  class MemoryConsumerRegistry;

  // Declared in construction order; destroyed in reverse.
  base::ScopedClosureRunner shutdown_thread_pool_;
  std::unique_ptr<blink::scheduler::WebThreadScheduler> main_thread_scheduler_;
  std::unique_ptr<base::ScopedMemoryConsumerRegistry<MemoryConsumerRegistry>>
      memory_consumer_registry_;
  std::unique_ptr<discardable_memory::DiscardableSharedMemoryManager>
      discardable_manager_;
  base::ScopedClosureRunner shutdown_scheduler_;
  std::unique_ptr<ShotPlatform> platform_;
  // Last, so it is the first thing torn down: the disk cache and the host
  // resolver post to the thread pool, and the sockets are watched by this
  // thread's message pump, so neither may still be running when those go.
  std::unique_ptr<ShotNetwork> network_;
};

}  // namespace shot

#endif  // SHOT_SHOT_RUNTIME_H_
