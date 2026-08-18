// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/controller/blink_leak_detector.h"

#include "base/dcheck_is_on.h"
#include "base/task/single_thread_task_runner.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/core_initializer.h"
#include "third_party/blink/renderer/core/css/css_default_style_sheets.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/editing/spellcheck/spell_checker.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/blink/renderer/platform/instrumentation/instance_counters.h"
#include "third_party/blink/renderer/platform/loader/fetch/memory_cache.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/scheduler/public/main_thread.h"
#include "third_party/blink/renderer/platform/scheduler/public/main_thread_scheduler.h"

namespace blink {

BlinkLeakDetector::BlinkLeakDetector(
    base::PassKey<BlinkLeakDetector> pass_key,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : delayed_gc_timer_(std::move(task_runner),
                        this,
                        &BlinkLeakDetector::TimerFiredGC) {}

BlinkLeakDetector::~BlinkLeakDetector() = default;

// static
void BlinkLeakDetector::Bind(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    mojo::PendingReceiver<mojom::blink::LeakDetector> receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<BlinkLeakDetector>(base::PassKey<BlinkLeakDetector>(),
                                          task_runner),
      std::move(receiver), task_runner);
}

void BlinkLeakDetector::PerformLeakDetection(
    PerformLeakDetectionCallback callback) {
  callback_ = std::move(callback);

  // This block dropped V8's internal caches and cycled
  // V8PerIsolateData's script regexp context, so that the static ScriptRegexp
  // that EmailInputType creates on first use did not perturb the object count.
  // ScriptRegexp is gone -- <input type=email> validation is implemented
  // directly against its grammar now -- so there is no context to cycle, and no
  // isolate to clear caches on. Evicting the memory cache still matters and
  // stays.
  MemoryCache::Get()->EvictResources();

  // Clear lazily loaded style sheets.
  CSSDefaultStyleSheets::Instance().PrepareForLeakDetection();

  // Stop keepalive loaders that may persist after page navigation.
  for (auto& resource_fetcher : ResourceFetcher::MainThreadFetchers()) {
    resource_fetcher->PrepareForLeakDetection();
  }

  Page::PrepareForLeakDetection();

  // A worker-thread-count bail-out was here: if any worker threads were
  // still running, leak detection aborted (via ReportInvalidResult(),
  // deleted with it) rather than race their synchronous destruction (see
  // https://crbug.com/1221158). There are no workers --
  // core/workers/WorkerThread no longer exists -- so that count is always
  // zero and the whole branch was dead.

  // Task queue may contain delayed object destruction tasks.
  // This method is called from navigation hook inside FrameLoader,
  // so previous document is still held by the loader until the next event loop.
  // Complete all pending tasks before proceeding to gc.
  number_of_gc_needed_ = 3;
  delayed_gc_timer_.StartOneShot(base::TimeDelta(), FROM_HERE);
}

void BlinkLeakDetector::TimerFiredGC(TimerBase*) {
  // Multiple rounds of GC are necessary as collectors may have postponed
  // clean-up tasks to the next event loop. E.g. the third GC is necessary for
  // cleaning up Document after the worker object has been reclaimed.

  ThreadState::Current()->CollectAllGarbageForTesting();
  CoreInitializer::GetInstance()
      .CollectAllGarbageForAnimationAndPaintWorkletForTesting();
  // Note: Oilpan precise GC is scheduled at the end of the event loop.

  // Inspect counters on the next event loop.
  if (--number_of_gc_needed_ > 0) {
    delayed_gc_timer_.StartOneShot(base::TimeDelta(), FROM_HERE);
  } else {
    // An extra pass keyed on DedicatedWorkerMessagingProxy::ProxyCount() was
    // here, to let posted worker-proxy finalization tasks run before the
    // final GC. DedicatedWorkerMessagingProxy no longer exists -- there are
    // no workers -- so that count is always zero and the extra pass never
    // fired.
    ReportResult();
  }
}

// ReportInvalidResult() definition was here; deleted along with the
// declaration in blink_leak_detector.h -- see the comment there.

void BlinkLeakDetector::ReportResult() {
  // Run with --enable-leak-detection-heap-snapshot (in addition to
  // --enable-leak-detection) to dunp a heap snapshot to file named
  // "leak_detection.heapsnapshot". This requires --no-sandbox, otherwise the
  // write to the file is blocked.
  // The --enable-leak-detection-heap-snapshot dump was here:
  // ThreadState::TakeHeapSnapshotForTesting() no longer exists (it is gone
  // from platform/heap/thread_state), so there is no snapshot to take
  // regardless of the switch.

  mojom::blink::LeakDetectionResultPtr result =
      mojom::blink::LeakDetectionResult::New();
  result->number_of_live_audio_nodes =
      InstanceCounters::CounterValue(InstanceCounters::kAudioHandlerCounter);
  result->number_of_live_documents =
      InstanceCounters::CounterValue(InstanceCounters::kDocumentCounter);
  result->number_of_live_nodes =
      InstanceCounters::CounterValue(InstanceCounters::kNodeCounter);
  result->number_of_live_layout_objects =
      InstanceCounters::CounterValue(InstanceCounters::kLayoutObjectCounter);
  result->number_of_live_resources =
      InstanceCounters::CounterValue(InstanceCounters::kResourceCounter);
  result->number_of_live_context_lifecycle_state_observers =
      InstanceCounters::CounterValue(
          InstanceCounters::kContextLifecycleStateObserverCounter);
  result->number_of_live_frames =
      InstanceCounters::CounterValue(InstanceCounters::kFrameCounter);
  result->number_of_live_v8_per_context_data = InstanceCounters::CounterValue(
      InstanceCounters::kV8PerContextDataCounter);
  result->number_of_worker_global_scopes = InstanceCounters::CounterValue(
      InstanceCounters::kWorkerGlobalScopeCounter);
  result->number_of_live_ua_css_resources =
      InstanceCounters::CounterValue(InstanceCounters::kUACSSResourceCounter);
  result->number_of_live_resource_fetchers =
      InstanceCounters::CounterValue(InstanceCounters::kResourceFetcherCounter);

#if DCHECK_IS_ON()
  ShowLiveDocumentInstances();
#endif

  std::move(callback_).Run(std::move(result));
}

}  // namespace blink
