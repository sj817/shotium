/*
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/timing/performance_user_timing.h"

#include "base/time/time.h"
#include "third_party/blink/public/mojom/use_counter/metrics/web_feature.mojom-shared.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_double_string.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/performance_entry_names.h"
#include "third_party/blink/renderer/core/timing/performance.h"
#include "third_party/blink/renderer/core/timing/performance_mark.h"
#include "third_party/blink/renderer/core/timing/performance_measure.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/bindings/source_location.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"

namespace blink {

UserTiming::UserTiming(Performance& performance) : performance_(&performance) {}

// AddMarkToPerformanceTimeline(PerformanceMark&, PerformanceMarkOptions*)
// used to live here: it inserted the mark into marks_map_/marks_buffer_ and,
// if blink.user_timing tracing was enabled, emitted a TRACE_EVENT_INSTANT
// for it (via the now-deleted InspectorTraceEvents::GetNextSampleTraceId()
// and PerformanceTiming::WriteInto()). It had no caller anywhere in the
// tree: Performance::mark() -- the only thing that could produce a
// PerformanceMark to pass in -- doesn't exist any more (see performance.idl:
// its `detail` argument is an arbitrary V8 value and can't be expressed
// without V8), so nothing ever calls this. Deleted along with its helpers.

void UserTiming::ClearMarks(const AtomicString& mark_name) {
  ClearPerformanceEntries(marks_map_, marks_buffer_, mark_name);
}

const PerformanceMark* UserTiming::FindExistingMark(
    const AtomicString& mark_name) {
  PerformanceEntryMap::const_iterator existing_marks =
      marks_map_.find(mark_name);
  if (existing_marks != marks_map_.end()) {
    PerformanceEntry* entry = existing_marks->value.back().Get();
    DCHECK(entry->entryType() == performance_entry_names::kMark);
    return static_cast<PerformanceMark*>(entry);
  }
  return nullptr;
}

double UserTiming::FindExistingMarkStartTime(const AtomicString& mark_name,
                                             ExceptionState& exception_state) {
  const PerformanceMark* mark = FindExistingMark(mark_name);
  if (mark) {
    return mark->startTime();
  }

  // Although there was no mark with the given name in UserTiming, we need to
  // support measuring with respect to |PerformanceTiming| attributes.
  if (!PerformanceTiming::IsAttributeName(mark_name)) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kSyntaxError,
        StrCat({"The mark '", mark_name, "' does not exist."}));
    return 0.0;
  }

  PerformanceTiming* timing = performance_->timing();
  if (!timing) {
    // According to
    // https://w3c.github.io/user-timing/#convert-a-name-to-a-timestamp.
    exception_state.ThrowTypeError(
        StrCat({"When converting a mark name ('", mark_name,
                "') to a timestamp given a name that is a read only attribute "
                "in the PerformanceTiming interface, the global object has to "
                "be a Window object."}));
    return 0.0;
  }

  // Because we know |PerformanceTiming::IsAttributeName(mark_name)| is true
  // (from above), we know calling |GetNamedAttribute| won't fail.
  double value = static_cast<double>(timing->GetNamedAttribute(mark_name));
  if (!value) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kInvalidAccessError,
        StrCat({"'", mark_name,
                "' is empty: either the event hasn't happened yet, or it would "
                "provide cross-origin timing information."}));
    return 0.0;
  }

  // Count the usage of PerformanceTiming attribute names in performance
  // measure. See crbug.com/1318445.
  blink::UseCounter::Count(performance_->GetExecutionContext(),
                           WebFeature::kPerformanceMeasureFindExistingName);

  return value - timing->navigationStart();
}

double UserTiming::GetTimeOrFindMarkTime(
    const AtomicString& measure_name,
    const V8UnionDoubleOrString* mark_or_time,
    ExceptionState& exception_state) {
  DCHECK(mark_or_time);

  switch (mark_or_time->GetContentType()) {
    case V8UnionDoubleOrString::ContentType::kDouble: {
      const double time = mark_or_time->GetAsDouble();
      if (time < 0.0) {
        exception_state.ThrowTypeError(StrCat(
            {"'", measure_name, "' cannot have a negative time stamp."}));
      }
      return time;
    }
    case V8UnionDoubleOrString::ContentType::kString:
      return FindExistingMarkStartTime(
          AtomicString(mark_or_time->GetAsString()), exception_state);
  }

  NOTREACHED();
}

base::TimeTicks UserTiming::GetPerformanceMarkUnsafeTimeForTraces(
    double start_time,
    const V8UnionDoubleOrString* maybe_mark_name) {
  if (maybe_mark_name && maybe_mark_name->IsString()) {
    const PerformanceMark* mark =
        FindExistingMark(AtomicString(maybe_mark_name->GetAsString()));
    if (mark) {
      return mark->UnsafeTimeForTraces();
    }
  }
  return performance_->GetTimeOriginInternal() + base::Milliseconds(start_time);
}

void UserTiming::ClearMeasures(const AtomicString& measure_name) {
  ClearPerformanceEntries(measures_map_, measures_buffer_, measure_name);
}

PerformanceEntryVector UserTiming::GetMarks() const {
  return marks_buffer_;
}

PerformanceEntryVector UserTiming::GetMarks(const AtomicString& name) const {
  PerformanceEntryMap::const_iterator it = marks_map_.find(name);
  if (it != marks_map_.end()) {
    return PerformanceEntryVector(it->value);
  }
  return {};
}

PerformanceEntryVector UserTiming::GetMeasures() const {
  return measures_buffer_;
}

PerformanceEntryVector UserTiming::GetMeasures(const AtomicString& name) const {
  PerformanceEntryMap::const_iterator it = measures_map_.find(name);
  if (it != measures_map_.end()) {
    return PerformanceEntryVector(it->value);
  }
  return {};
}

void UserTiming::InsertPerformanceEntry(
    PerformanceEntryMap& performance_entry_map,
    PerformanceEntryVector& performance_entry_buffer,
    PerformanceEntry& entry) {
  performance_->InsertEntryIntoSortedBuffer(performance_entry_buffer, entry);

  auto it = performance_entry_map.find(entry.name());
  if (it == performance_entry_map.end()) {
    performance_entry_map.Set(entry.name(), PerformanceEntryVector({&entry}));
    return;
  }

  performance_->InsertEntryIntoSortedBuffer(it->value, entry);
}

void UserTiming::ClearPerformanceEntries(
    PerformanceEntryMap& performance_entry_map,
    PerformanceEntryVector& performance_entry_buffer,
    const AtomicString& name) {
  if (name.IsNull()) {
    performance_entry_map.clear();
    performance_entry_buffer.clear();
    return;
  }

  if (performance_entry_map.Contains(name)) {
    UseCounter::Count(performance_->GetExecutionContext(),
                      WebFeature::kClearPerformanceEntries);

    // Remove key/value pair from the map.
    performance_entry_map.erase(name);

    // In favor of quicker getEntries() calls, we tradeoff performance here to
    // linearly 'clear' entries in the vector.
    performance_entry_buffer.erase(
        std::remove_if(performance_entry_buffer.begin(),
                       performance_entry_buffer.end(),
                       [name](auto& entry) { return entry->name() == name; }),
        performance_entry_buffer.end());
  }
}

void UserTiming::Trace(Visitor* visitor) const {
  visitor->Trace(performance_);
  visitor->Trace(marks_map_);
  visitor->Trace(measures_map_);
  visitor->Trace(marks_buffer_);
  visitor->Trace(measures_buffer_);
}

}  // namespace blink
