// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/web/blink.h"

namespace blink {

// Function defined in third_party/blink/public/web/blink.h.
void LogStatsDuringShutdown() {
  // WARNING: this code path is *not* hit during fast shutdown.

  // Everything this reported came out of V8: isolate->DumpAndResetStats() and,
  // under --dump-blink-runtime-call-stats, LogRuntimeCallStats(). Both the
  // isolates and RuntimeCallStats are gone, so there are no stats to log.
  // The function stays because public/web/blink.h declares it and the embedder
  // calls it on the shutdown path.
}

}  // namespace blink
