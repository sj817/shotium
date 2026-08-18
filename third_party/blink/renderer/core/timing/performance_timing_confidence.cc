// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/performance_timing_confidence.h"


namespace blink {

PerformanceTimingConfidence::PerformanceTimingConfidence(
    double randomizedTriggerRate,
    V8PerformanceTimingConfidenceValue value)
    : randomizedTriggerRate_(randomizedTriggerRate), value_(value) {}

}  // namespace blink
