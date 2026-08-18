// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_GLOBAL_PERFORMANCE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_GLOBAL_PERFORMANCE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/timing/window_performance.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"

namespace blink {

class LocalDOMWindow;

class CORE_EXPORT GlobalPerformance {
  STATIC_ONLY(GlobalPerformance);

 public:
  static WindowPerformance* performance(LocalDOMWindow&);
  // performance(WorkerGlobalScope&) used to live here, backing
  // self.performance inside a Web Worker. //content is gone and with it any
  // way to spin up a worker thread -- core/workers/ itself has been deleted
  // -- so WorkerGlobalScope is now only an incomplete forward declaration
  // with no implementation anywhere in the tree, and WorkerPerformance (the
  // Performance subclass it returned) went with it. See
  // worker_performance.h/.cc and worker_global_scope_performance.h/.cc in
  // git history.
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_GLOBAL_PERFORMANCE_H_
