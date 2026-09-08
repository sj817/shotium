// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/performance_measure.h"

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/performance_entry_names.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"

namespace blink {

const AtomicString& PerformanceMeasure::entryType() const {
  return performance_entry_names::kMeasure;
}

PerformanceEntryType PerformanceMeasure::EntryTypeEnum() const {
  return PerformanceEntry::EntryType::kMeasure;
}

void PerformanceMeasure::Trace(Visitor* visitor) const {
  PerformanceEntry::Trace(visitor);
}

}  // namespace blink
