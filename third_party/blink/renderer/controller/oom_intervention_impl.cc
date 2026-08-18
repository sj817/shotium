// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/controller/oom_intervention_impl.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/debug/crash_logging.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/single_thread_task_runner.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/heap/thread_state.h"
#include "third_party/blink/renderer/platform/scheduler/public/main_thread.h"
#include "third_party/blink/renderer/platform/scheduler/public/main_thread_scheduler.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

namespace {

base::debug::CrashKeyString* GetStateCrashKey() {
  static auto* crash_key = base::debug::AllocateCrashKeyString(
      "oom_intervention_state", base::debug::CrashKeySize::Size32);
  return crash_key;
}

// NavigateLocalAdsFrames() was here: on near-OOM it navigated every ad-tagged
// child frame to about:blank. Nothing in this build can tag a frame as an ad,
// so the walk had nothing to find.

}  // namespace

// static
void OomInterventionImpl::BindReceiver(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    mojo::PendingReceiver<mojom::blink::OomIntervention> receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<OomInterventionImpl>(
          base::PassKey<OomInterventionImpl>(), task_runner),
      std::move(receiver), task_runner);
}

OomInterventionImpl::OomInterventionImpl(
    base::PassKey<OomInterventionImpl> pass_key,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : OomInterventionImpl(std::move(task_runner)) {}

OomInterventionImpl::OomInterventionImpl(
    scoped_refptr<base::SingleThreadTaskRunner> task_runner)
    : task_runner_(std::move(task_runner)) {
  static bool initial_crash_key_set = false;
  if (!initial_crash_key_set) {
    initial_crash_key_set = true;
    base::debug::SetCrashKeyString(GetStateCrashKey(), "before");
  }
}

OomInterventionImpl::~OomInterventionImpl() {
  MemoryUsageMonitorInstance().RemoveObserver(this);
}

void OomInterventionImpl::StartDetection(
    mojo::PendingRemote<mojom::blink::OomInterventionHost> host,
    mojom::blink::DetectionArgsPtr detection_args,
    bool renderer_pause_enabled,
    bool purge_v8_memory_enabled) {
  host_.Bind(std::move(host));

  detection_args_ = std::move(detection_args);
  renderer_pause_enabled_ = renderer_pause_enabled;
  purge_v8_memory_enabled_ = purge_v8_memory_enabled;

  MemoryUsageMonitorInstance().AddObserver(this);
}

MemoryUsageMonitor& OomInterventionImpl::MemoryUsageMonitorInstance() {
  return MemoryUsageMonitor::Instance();
}

void OomInterventionImpl::OnMemoryPing(MemoryUsage usage) {
  // Ignore pings without process memory usage information.
  if (std::isnan(usage.private_footprint_bytes) ||
      std::isnan(usage.swap_bytes) || std::isnan(usage.vm_size_bytes))
    return;
  Check(usage);
}

void OomInterventionImpl::Check(MemoryUsage usage) {
  DCHECK(host_);

  bool oom_detected = false;

  oom_detected |= detection_args_->private_footprint_threshold > 0 &&
                  usage.private_footprint_bytes >
                      detection_args_->private_footprint_threshold;

  if (oom_detected) {
    base::debug::SetCrashKeyString(GetStateCrashKey(), "during");

    // Both of this intervention's other legs are gone: purge_v8_memory_enabled_
    // drove LocalFrame::ForciblyPurgeV8Memory(), which went with V8, and
    // navigate_ads_enabled_ navigated ad frames away, which went with ad
    // tagging. Pausing the renderer is what is left.
    if (renderer_pause_enabled_) {
      // The ScopedPagePauser is destroyed when the intervention is declined and
      // mojo strong binding is disconnected.
      pauser_ = std::make_unique<ScopedPagePauser>();
    }

    host_->OnHighMemoryUsage();
    MemoryUsageMonitorInstance().RemoveObserver(this);
    // Send memory pressure notification to trigger GC.
    task_runner_->PostTask(FROM_HERE, BindOnce(&TriggerGC));
    // V8GCForContextDispose was told here to force a GC on the next page
    // navigation, since memory being high is exactly when that matters. It was
    // a V8 context-disposal heuristic and went with V8; the forced collection
    // posted just above is what remains, and it is the collection that actually
    // frees the DOM.
  }
}

void OomInterventionImpl::TriggerGC() {
  // This was a critical memory pressure notification to every main thread V8
  // isolate. The equivalent for the surviving heap is a forced cppgc
  // collection, which is what actually holds the DOM under memory pressure.
  ThreadState::Current()->CollectAllGarbageForMemoryPressure();
}

}  // namespace blink
