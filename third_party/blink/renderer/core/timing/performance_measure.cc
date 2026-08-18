// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/performance_measure.h"

#include "third_party/blink/public/mojom/timing/performance_mark_or_measure.mojom-blink.h"
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

mojom::blink::PerformanceMarkOrMeasurePtr
PerformanceMeasure::ToMojoPerformanceMarkOrMeasure() {
  // `detail` used to be a V8-serialized `any` the script passed to
  // performance.measure(); that serialization went with V8, and
  // Performance::measure() itself is gone (see performance.idl), so there is
  // no detail value to forward here. `detail` stays unset, which the mojom
  // declares as a nullable field.
  return PerformanceEntry::ToMojoPerformanceMarkOrMeasure();
}

void PerformanceMeasure::Trace(Visitor* visitor) const {
  // deserialized_detail_map_ was the per-isolate cache of the V8-deserialized
  // `detail` value; it went with V8 serialization (see
  // ToMojoPerformanceMarkOrMeasure() above), so there is nothing left to
  // trace here beyond the base class.
  PerformanceEntry::Trace(visitor);
}

}  // namespace blink
