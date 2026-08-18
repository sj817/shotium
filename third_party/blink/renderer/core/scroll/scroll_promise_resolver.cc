// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/v8_scroll_result.h"
#include "third_party/blink/renderer/core/scroll/scroll_promise_resolver.h"

namespace blink {

ScrollPromiseResolver::ScrollPromiseResolver() = default;

ScrollPromiseResolver::~ScrollPromiseResolver() {
  CHECK_EQ(num_active_scrolls_, 0U);
}

void ScrollPromiseResolver::Trace(Visitor* visitor) const {}

std::unique_ptr<ScrollPromiseResolver::ActiveScrollTracker>
ScrollPromiseResolver::CreateActiveScrollTracker() {
  num_active_scrolls_++;
  return std::make_unique<ActiveScrollTracker>(this);
}

void ScrollPromiseResolver::ActiveScrollTrackerRemoved() {
  CHECK_GT(num_active_scrolls_, 0U);
  --num_active_scrolls_;
}

}  // namespace blink
