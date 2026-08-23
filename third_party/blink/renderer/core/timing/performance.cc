/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 * Copyright (C) 2012 Intel Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/timing/performance.h"

#include <algorithm>
#include <optional>

#include "base/check_op.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/default_clock.h"
#include "base/time/time.h"
#include "base/values.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/mojom/permissions_policy/document_policy_feature.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_double_string.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/document_timing.h"
#include "third_party/blink/renderer/core/dom/dom_exception.h"
#include "third_party/blink/renderer/core/dom/dom_high_res_time_stamp.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/event_target_names.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/loader/document_load_timing.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/probe/core_probes.h"
#include "third_party/blink/renderer/core/timing/back_forward_cache_restoration.h"
#include "third_party/blink/renderer/core/timing/background_tracing_helper.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/interaction_contentful_paint.h"
#include "third_party/blink/renderer/core/timing/largest_contentful_paint.h"
#include "third_party/blink/renderer/core/timing/layout_shift.h"
#include "third_party/blink/renderer/core/timing/performance_container_timing.h"
#include "third_party/blink/renderer/core/timing/performance_element_timing.h"
#include "third_party/blink/renderer/core/timing/performance_entry.h"
#include "third_party/blink/renderer/core/timing/performance_event_timing.h"
#include "third_party/blink/renderer/core/timing/performance_long_task_timing.h"
#include "third_party/blink/renderer/core/timing/performance_mark.h"
#include "third_party/blink/renderer/core/timing/performance_measure.h"
#include "third_party/blink/renderer/core/timing/performance_navigation_timing.h"
#include "third_party/blink/renderer/core/timing/performance_paint_timing.h"
#include "third_party/blink/renderer/core/timing/performance_resource_timing.h"
#include "third_party/blink/renderer/core/timing/performance_scroll_timing.h"
#include "third_party/blink/renderer/core/timing/performance_server_timing.h"
#include "third_party/blink/renderer/core/timing/performance_soft_navigation.h"
#include "third_party/blink/renderer/core/timing/performance_user_timing.h"
#include "third_party/blink/renderer/core/timing/time_clamper.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_load_timing.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_timing_utils.h"
#include "third_party/blink/renderer/platform/network/http_parsers.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

namespace {

// LongTask API can be a source of many events. Filter on Performance object
// level before reporting to UKM to smooth out recorded events over all pages.
constexpr size_t kLongTaskUkmSampleInterval = 100;

// Three UMA histogram names lived here -- kParserPausingCalledAfterResumimg,
// kParserResumeByUserTiming and kParserResumingCalledBeforePausing. All three
// measured the parser being paused and resumed by performance.mark()/measure()
// calls in the page. User timing is a script API; nothing pauses the parser
// that way any more, so nothing recorded them.

// IsMeasureOptionsEmpty(const PerformanceMeasureOptions&) used to live here,
// checking whether an options bag passed to performance.measure() was
// effectively unset. performance.measure()'s `detail` is an arbitrary V8
// value, so neither the options bag nor Performance::measure() itself can
// exist without V8 (see the comment in performance.idl); this helper had no
// caller left once measure() was removed in the v8ectomy pass.

base::TimeDelta GetUnixAtZeroMonotonic(const base::Clock* clock) {
  base::TimeDelta unix_time_now = clock->Now() - base::Time::UnixEpoch();
  base::TimeDelta time_since_origin = base::TimeTicks::Now().since_origin();
  return unix_time_now - time_since_origin;
}

void RecordLongTaskUkm(ExecutionContext* execution_context,
                       base::TimeDelta start_time,
                       base::TimeDelta duration) {
  // The V8 breakdown of the long task (GC and execute time) is gone with
  // the engine; the task's own start time and duration are still recorded.
  ukm::builders::PerformanceAPI_LongTask(execution_context->UkmSourceID())
      .SetStartTime(start_time.InMilliseconds())
      .SetDuration(duration.InMicroseconds())
      .Record(execution_context->UkmRecorder());
}

PerformanceEntry::EntryType kDroppableEntryTypes[] = {
    PerformanceEntry::kResource,
    PerformanceEntry::kLongTask,
    PerformanceEntry::kElement,
    PerformanceEntry::kEvent,
    PerformanceEntry::kLayoutShift,
    PerformanceEntry::kLargestContentfulPaint,
    PerformanceEntry::kPaint,
    PerformanceEntry::kBackForwardCacheRestoration,
    PerformanceEntry::kSoftNavigation,
    PerformanceEntry::kInteractionContentfulPaint,
    PerformanceEntry::kScroll,
};

void SwapEntries(PerformanceEntryVector& entries,
                 int leftIndex,
                 int rightIndex) {
  auto tmp = entries[leftIndex];
  entries[leftIndex] = entries[rightIndex];
  entries[rightIndex] = tmp;
}

inline bool CheckName(const PerformanceEntry* entry,
                      const AtomicString& maybe_name) {
  // If we're not filtering by name, then any entry matches.
  if (!maybe_name) {
    return true;
  }
  return entry->name() == maybe_name;
}

// NotifyParserResume() was here, telling the document that user timing had
// resumed its parser and recording whether it had. See the histogram comment
// at the top of this namespace.

}  // namespace

PerformanceEntryVector MergePerformanceEntryVectors(
    const PerformanceEntryVector& first_entry_vector,
    const PerformanceEntryVector& second_entry_vector,
    const AtomicString& maybe_name) {
  PerformanceEntryVector merged_entries;
  merged_entries.reserve(first_entry_vector.size() +
                         second_entry_vector.size());

  auto first_it = first_entry_vector.CheckedBegin();
  auto first_end = first_entry_vector.CheckedEnd();
  auto second_it = second_entry_vector.CheckedBegin();
  auto second_end = second_entry_vector.CheckedEnd();

  // Advance the second iterator past any entries with disallowed names.
  while (second_it != second_end && !CheckName(*second_it, maybe_name)) {
    ++second_it;
  }

  auto PushBackSecondIteratorAndAdvance = [&]() {
    DCHECK(CheckName(*second_it, maybe_name));
    merged_entries.push_back(*second_it);
    ++second_it;
    while (second_it != second_end && !CheckName(*second_it, maybe_name)) {
      ++second_it;
    }
  };

  // What follows is based roughly on a reference implementation of std::merge,
  // except that after copying a value from the second iterator, it must also
  // advance the second iterator past any entries with disallowed names.

  while (first_it != first_end) {
    // If the second iterator has ended, just copy the rest of the contents
    // from the first iterator.
    if (second_it == second_end) {
      std::copy(first_it, first_end, std::back_inserter(merged_entries));
      break;
    }

    // Add an entry to the result vector from either the first or second
    // iterator, whichever has an earlier time. The first iterator wins ties.
    if (PerformanceEntry::StartTimeCompareLessThan(*second_it, *first_it)) {
      PushBackSecondIteratorAndAdvance();
    } else {
      DCHECK(CheckName(*first_it, maybe_name));
      merged_entries.push_back(*first_it);
      ++first_it;
    }
  }

  // If there are still entries in the second iterator after the first iterator
  // has ended, copy all remaining entries that have allowed names.
  while (second_it != second_end) {
    PushBackSecondIteratorAndAdvance();
  }

  return merged_entries;
}

constexpr size_t kDefaultResourceTimingBufferSize = 250;
constexpr size_t kDefaultEventTimingBufferSize = 150;
constexpr size_t kDefaultContainerTimingBufferSize = 150;
constexpr size_t kDefaultElementTimingBufferSize = 150;
constexpr size_t kDefaultScrollTimingBufferSize = 150;
constexpr size_t kDefaultLayoutShiftBufferSize = 150;
constexpr size_t kDefaultLargestContenfulPaintSize = 150;
constexpr size_t kDefaultInteractionContenfulPaintSize = 150;
constexpr size_t kDefaultLongTaskBufferSize = 200;
constexpr size_t kDefaultLongAnimationFrameBufferSize = 200;
constexpr size_t kDefaultBackForwardCacheRestorationBufferSize = 200;
constexpr size_t kDefaultSoftNavigationBufferSize = 50;
// Paint timing entries is more than twice as much as the soft navigation buffer
// size, as there can be 2 paint entries for each soft navigation, plus 2
// entries for the initial navigation.
constexpr size_t kDefaultPaintEntriesBufferSize =
    kDefaultSoftNavigationBufferSize * 2 + 2;

Performance::Performance(
    base::TimeTicks time_origin,
    bool cross_origin_isolated_capability,
    scoped_refptr<base::SingleThreadTaskRunner> task_runner,
    ExecutionContext* context)
    : resource_timing_buffer_size_limit_(kDefaultResourceTimingBufferSize),
      back_forward_cache_restoration_buffer_size_limit_(
          kDefaultBackForwardCacheRestorationBufferSize),
      event_timing_buffer_max_size_(kDefaultEventTimingBufferSize),
      container_timing_buffer_max_size_(kDefaultContainerTimingBufferSize),
      element_timing_buffer_max_size_(kDefaultElementTimingBufferSize),
      scroll_timing_buffer_max_size_(kDefaultScrollTimingBufferSize),
      user_timing_(nullptr),
      time_origin_(time_origin),
      cross_origin_isolated_capability_(cross_origin_isolated_capability),
      observer_filter_options_(PerformanceEntry::kInvalid),
      task_runner_(std::move(task_runner)),
      deliver_observations_timer_(task_runner_,
                                  this,
                                  &Performance::DeliverObservationsTimerFired),
      resource_timing_buffer_full_timer_(
          task_runner_,
          this,
          &Performance::FireResourceTimingBufferFull),
      performance_entries_flush_timer_(
          task_runner_,
          this,
          &Performance::PerformanceEntriesFlushTimerFired),
      declarative_performance_observer_host_(context) {
  unix_at_zero_monotonic_ =
      GetUnixAtZeroMonotonic(base::DefaultClock::GetInstance());
  // |context| may be null in tests.
  if (context) {
    background_tracing_helper_ =
        MakeGarbageCollected<BackgroundTracingHelper>(context);
  }
  // Initialize the map of dropped entry types only with those which could be
  // dropped (saves some unnecessary 0s).
  for (const auto type : kDroppableEntryTypes) {
    dropped_entries_count_map_.insert(type, 0);
  }
}

Performance::~Performance() = default;

const AtomicString& Performance::InterfaceName() const {
  return event_target_names::kPerformance;
}

PerformanceTiming* Performance::timing() const {
  return nullptr;
}

PerformanceNavigation* Performance::navigation() const {
  return nullptr;
}

EventCounts* Performance::eventCounts() {
  return nullptr;
}

DOMHighResTimeStamp Performance::timeOrigin() const {
  DCHECK(!time_origin_.is_null());
  base::TimeDelta time_origin_from_zero_monotonic =
      time_origin_ - base::TimeTicks();
  return ClampTimeResolution(
      unix_at_zero_monotonic_ + time_origin_from_zero_monotonic,
      cross_origin_isolated_capability_);
}

PerformanceEntryVector Performance::getEntries() {
  return GetEntriesForCurrentFrame();
}

PerformanceEntryVector Performance::GetEntriesForCurrentFrame(
    const AtomicString& maybe_name) {
  PerformanceEntryVector entries;

  entries = MergePerformanceEntryVectors(entries, resource_timing_buffer_,
                                         maybe_name);
  if (first_input_timing_ && CheckName(first_input_timing_, maybe_name)) {
    InsertEntryIntoSortedBuffer(entries, *first_input_timing_);
  }
  // This extra checking is needed when WorkerPerformance
  // calls this method.
  if (navigation_timing_ && CheckName(navigation_timing_, maybe_name)) {
    InsertEntryIntoSortedBuffer(entries, *navigation_timing_);
  }

  if (paint_entries_timing_.size()) {
    entries = MergePerformanceEntryVectors(entries, paint_entries_timing_,
                                           maybe_name);
  }

  if (RuntimeEnabledFeatures::NavigationIdEnabled(GetExecutionContext())) {
    entries = MergePerformanceEntryVectors(
        entries, back_forward_cache_restoration_buffer_, maybe_name);
  }

  if (RuntimeEnabledFeatures::SoftNavigationHeuristicsEnabled(
          GetExecutionContext()) &&
      soft_navigation_buffer_.size()) {
    UseCounter::Count(GetExecutionContext(),
                      WebFeature::kSoftNavigationHeuristics);
    entries = MergePerformanceEntryVectors(entries, soft_navigation_buffer_,
                                           maybe_name);
  }

  if (long_animation_frame_buffer_.size()) {
    entries = MergePerformanceEntryVectors(
        entries, long_animation_frame_buffer_, maybe_name);
  }

  if (visibility_state_buffer_.size()) {
    entries = MergePerformanceEntryVectors(entries, visibility_state_buffer_,
                                           maybe_name);
  }

  // `user_timing_` is the largest in size, in order to keep
  // `MergePerformanceEntryVectors` performant, carry out the merge in
  // the end.
  if (user_timing_) {
    if (maybe_name) {
      // UserTiming already stores lists of marks and measures by name, so
      // requesting them directly is much more efficient than getting the full
      // lists of marks and measures and then filtering during the merge.
      entries = MergePerformanceEntryVectors(
          entries, user_timing_->GetMarks(maybe_name), g_null_atom);
      entries = MergePerformanceEntryVectors(
          entries, user_timing_->GetMeasures(maybe_name), g_null_atom);
    } else {
      entries = MergePerformanceEntryVectors(entries, user_timing_->GetMarks(),
                                             g_null_atom);
      entries = MergePerformanceEntryVectors(
          entries, user_timing_->GetMeasures(), g_null_atom);
    }
  }

  return entries;
}

PerformanceEntryVector Performance::getBufferedEntriesByType(
    const AtomicString& entry_type) {
  PerformanceEntry::EntryType type =
      PerformanceEntry::ToEntryTypeEnum(entry_type);
  return getEntriesByTypeInternal(type, /*maybe_name=*/g_null_atom);
}

PerformanceEntryVector Performance::getEntriesByType(
    const AtomicString& entry_type) {
  return GetEntriesByTypeForCurrentFrame(entry_type);
}

PerformanceEntryVector Performance::GetEntriesByTypeForCurrentFrame(
    const AtomicString& entry_type,
    const AtomicString& maybe_name) {
  PerformanceEntry::EntryType type =
      PerformanceEntry::ToEntryTypeEnum(entry_type);
  if (!PerformanceEntry::IsValidTimelineEntryType(type)) {
    PerformanceEntryVector empty_entries;
    if (ExecutionContext* execution_context = GetExecutionContext()) {
      String message = "Deprecated API for given entry type.";
      execution_context->AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
          mojom::ConsoleMessageSource::kJavaScript,
          mojom::ConsoleMessageLevel::kWarning, message));
    }
    return empty_entries;
  }
  return getEntriesByTypeInternal(type, maybe_name);
}

PerformanceEntryVector Performance::getEntriesByTypeInternal(
    PerformanceEntry::EntryType type,
    const AtomicString& maybe_name) {
  // This vector may be used by any cases below which require local storage.
  // Cases which refer to pre-existing vectors may simply set `entries` instead.
  PerformanceEntryVector entries_storage;

  PerformanceEntryVector* entries = &entries_storage;
  bool already_filtered_by_name = false;
  switch (type) {
    case PerformanceEntry::kResource:
      UseCounter::Count(GetExecutionContext(), WebFeature::kResourceTiming);
      entries = &resource_timing_buffer_;
      break;

    case PerformanceEntry::kContainer:
      entries = &container_timing_buffer_;
      break;

    case PerformanceEntry::kElement:
      entries = &element_timing_buffer_;
      break;

    case PerformanceEntry::kEvent:
      UseCounter::Count(GetExecutionContext(),
                        WebFeature::kEventTimingExplicitlyRequested);
      entries = &event_timing_buffer_;
      break;

    case PerformanceEntry::kFirstInput:
      UseCounter::Count(GetExecutionContext(),
                        WebFeature::kEventTimingExplicitlyRequested);
      UseCounter::Count(GetExecutionContext(),
                        WebFeature::kEventTimingFirstInputExplicitlyRequested);
      if (first_input_timing_)
        entries_storage = {first_input_timing_};
      break;

    case PerformanceEntry::kNavigation:
      UseCounter::Count(GetExecutionContext(), WebFeature::kNavigationTimingL2);
      if (navigation_timing_)
        entries_storage = {navigation_timing_};
      break;

    case PerformanceEntry::kMark:
      if (user_timing_) {
        if (maybe_name) {
          entries_storage = user_timing_->GetMarks(maybe_name);
          already_filtered_by_name = true;
        } else {
          entries_storage = user_timing_->GetMarks();
        }
      }
      break;

    case PerformanceEntry::kMeasure:
      if (user_timing_) {
        if (maybe_name) {
          entries_storage = user_timing_->GetMeasures(maybe_name);
          already_filtered_by_name = true;
        } else {
          entries_storage = user_timing_->GetMeasures();
        }
      }
      break;

    case PerformanceEntry::kPaint: {
      UseCounter::Count(GetExecutionContext(),
                        WebFeature::kPaintTimingRequested);
      entries = &paint_entries_timing_;
      break;
    }

    case PerformanceEntry::kLongTask:
      entries = &longtask_buffer_;
      break;

    // TaskAttribution & script entries are only associated to longtask entries.
    case PerformanceEntry::kTaskAttribution:
    case PerformanceEntry::kScript:
      break;

    // Scroll entries are buffered and observed via the PerformanceScrollTiming
    // API.
    case PerformanceEntry::kScroll:
      entries = &scroll_timing_buffer_;
      break;

    case PerformanceEntry::kLayoutShift:
      entries = &layout_shift_buffer_;
      break;

    case PerformanceEntry::kLargestContentfulPaint:
      entries = &largest_contentful_paint_buffer_;
      break;

    case PerformanceEntry::kInteractionContentfulPaint:
      entries = &interaction_contentful_paint_buffer_;
      break;

    case PerformanceEntry::kVisibilityState:
      entries = &visibility_state_buffer_;
      break;

    case PerformanceEntry::kBackForwardCacheRestoration:
      if (RuntimeEnabledFeatures::NavigationIdEnabled(GetExecutionContext()))
        entries = &back_forward_cache_restoration_buffer_;
      break;

    case PerformanceEntry::kSoftNavigation:
      if (RuntimeEnabledFeatures::SoftNavigationHeuristicsEnabled(
              GetExecutionContext())) {
        UseCounter::Count(GetExecutionContext(),
                          WebFeature::kSoftNavigationHeuristics);
        entries = &soft_navigation_buffer_;
      }
      break;

    case PerformanceEntry::kLongAnimationFrame:
        UseCounter::Count(GetExecutionContext(),
                          WebFeature::kLongAnimationFrameRequested);
        entries = &long_animation_frame_buffer_;
      break;

    // Conditional user timing entries are included in other relevant
    // Performance entries. They are not retrievable through Performance
    // interface.
    case PerformanceEntry::kMarkConditional:
    case PerformanceEntry::kMeasureConditional:
      break;

    case PerformanceEntry::kInvalid:
      break;
  }

  DCHECK_NE(entries, nullptr);
  if (!maybe_name || already_filtered_by_name) {
    return *entries;
  }

  PerformanceEntryVector filtered_entries;
  std::copy_if(entries->begin(), entries->end(),
               std::back_inserter(filtered_entries),
               [&](const PerformanceEntry* entry) {
                 return entry->name() == maybe_name;
               });
  return filtered_entries;
}

PerformanceEntryVector Performance::getEntriesByName(
    const AtomicString& name,
    const AtomicString& entry_type) {
  PerformanceEntryVector entries;

  // Get sorted entry list based on provided input.
  if (entry_type.IsNull()) {
    entries = GetEntriesForCurrentFrame(name);
  } else {
    entries = GetEntriesByTypeForCurrentFrame(entry_type, name);
  }

  return entries;
}

void Performance::clearResourceTimings() {
  resource_timing_buffer_.clear();
}

void Performance::setResourceTimingBufferSize(unsigned size) {
  resource_timing_buffer_size_limit_ = size;
}

void Performance::setBackForwardCacheRestorationBufferSizeForTest(
    unsigned size) {
  back_forward_cache_restoration_buffer_size_limit_ = size;
}

void Performance::setEventTimingBufferSizeForTest(unsigned size) {
  event_timing_buffer_max_size_ = size;
}

void Performance::AddResourceTiming(mojom::blink::ResourceTimingInfoPtr info,
                                    const AtomicString& initiator_type) {
  ExecutionContext* context = GetExecutionContext();
  auto* entry = MakeGarbageCollected<PerformanceResourceTiming>(
      std::move(info), initiator_type, time_origin_,
      cross_origin_isolated_capability_, context,
      NavigationId().web_exposed_id);
  NotifyObserversOfEntry(*entry);
  // https://w3c.github.io/resource-timing/#dfn-add-a-performanceresourcetiming-entry
  if (CanAddResourceTimingEntry() &&
      !resource_timing_buffer_full_event_pending_) {
    InsertEntryIntoSortedBuffer(resource_timing_buffer_, *entry);
    return;
  }

  // The Resource Timing entries have a special processing model in which there
  // is a secondary buffer but getting those entries requires handling the
  // buffer full event, and the PerformanceObserver with buffered flag only
  // receives the entries from the primary buffer, so it's ok to increase
  // the dropped entries count here.
  ++(dropped_entries_count_map_.find(PerformanceEntry::kResource)->value);
  if (!resource_timing_buffer_full_event_pending_) {
    resource_timing_buffer_full_event_pending_ = true;
    resource_timing_buffer_full_timer_.StartOneShot(base::TimeDelta(),
                                                    FROM_HERE);
  }
  resource_timing_secondary_buffer_.push_back(entry);
}

// Called after loadEventEnd happens.
void Performance::NotifyNavigationTimingToObservers() {
  if (navigation_timing_)
    NotifyObserversOfEntry(*navigation_timing_);
}

bool Performance::IsContainerTimingBufferFull() const {
  return container_timing_buffer_.size() >= container_timing_buffer_max_size_;
}

bool Performance::IsElementTimingBufferFull() const {
  return element_timing_buffer_.size() >= element_timing_buffer_max_size_;
}

bool Performance::IsEventTimingBufferFull() const {
  return event_timing_buffer_.size() >= event_timing_buffer_max_size_;
}

bool Performance::IsLongAnimationFrameBufferFull() const {
  return long_animation_frame_buffer_.size() >=
         kDefaultLongAnimationFrameBufferSize;
}

void Performance::CopySecondaryBuffer() {
  // https://w3c.github.io/resource-timing/#dfn-copy-secondary-buffer
  while (!resource_timing_secondary_buffer_.empty() &&
         CanAddResourceTimingEntry()) {
    PerformanceEntry* entry = resource_timing_secondary_buffer_.front();
    DCHECK(entry);
    resource_timing_secondary_buffer_.pop_front();
    resource_timing_buffer_.push_back(entry);
  }
}

void Performance::FireResourceTimingBufferFull(TimerBase*) {
  // https://w3c.github.io/resource-timing/#dfn-fire-a-buffer-full-event
  while (!resource_timing_secondary_buffer_.empty()) {
    int excess_entries_before = resource_timing_secondary_buffer_.size();
    if (!CanAddResourceTimingEntry()) {
      DispatchEvent(
          *Event::Create(event_type_names::kResourcetimingbufferfull));
    }
    CopySecondaryBuffer();
    int excess_entries_after = resource_timing_secondary_buffer_.size();
    if (excess_entries_after >= excess_entries_before) {
      resource_timing_secondary_buffer_.clear();
      break;
    }
  }
  resource_timing_buffer_full_event_pending_ = false;
}

void Performance::AddToContainerTimingBuffer(
    PerformanceContainerTiming& entry) {
  if (!IsContainerTimingBufferFull()) {
    InsertEntryIntoSortedBuffer(container_timing_buffer_, entry);
  } else {
    ++(dropped_entries_count_map_.find(PerformanceEntry::kContainer)->value);
  }
}

void Performance::AddToElementTimingBuffer(PerformanceElementTiming& entry) {
  if (!IsElementTimingBufferFull()) {
    InsertEntryIntoSortedBuffer(element_timing_buffer_, entry);
  } else {
    ++(dropped_entries_count_map_.find(PerformanceEntry::kElement)->value);
  }
}

bool Performance::IsScrollTimingBufferFull() const {
  return scroll_timing_buffer_.size() >= scroll_timing_buffer_max_size_;
}

void Performance::AddToScrollTimingBuffer(PerformanceScrollTiming& entry) {
  if (!IsScrollTimingBufferFull()) {
    InsertEntryIntoSortedBuffer(scroll_timing_buffer_, entry);
  } else {
    ++(dropped_entries_count_map_.find(PerformanceEntry::kScroll)->value);
  }
}

void Performance::AddToEventTimingBuffer(PerformanceEventTiming& entry) {
  if (!IsEventTimingBufferFull()) {
    InsertEntryIntoSortedBuffer(event_timing_buffer_, entry);
  } else {
    ++(dropped_entries_count_map_.find(PerformanceEntry::kEvent)->value);
  }
}

void Performance::AddToLayoutShiftBuffer(LayoutShift& entry) {
  probe::PerformanceEntryAdded(GetExecutionContext(), &entry);
  if (layout_shift_buffer_.size() < kDefaultLayoutShiftBufferSize) {
    InsertEntryIntoSortedBuffer(layout_shift_buffer_, entry);
  } else {
    ++(dropped_entries_count_map_.find(PerformanceEntry::kLayoutShift)->value);
  }
}

void Performance::AddLargestContentfulPaint(LargestContentfulPaint* entry) {
  probe::PerformanceEntryAdded(GetExecutionContext(), entry);
  if (largest_contentful_paint_buffer_.size() <
      kDefaultLargestContenfulPaintSize) {
    InsertEntryIntoSortedBuffer(largest_contentful_paint_buffer_, *entry);
  } else {
    ++(dropped_entries_count_map_
           .find(PerformanceEntry::kLargestContentfulPaint)
           ->value);
  }

  if (RuntimeEnabledFeatures::DeclarativePerformanceObserverEnabled(
          GetExecutionContext()) &&
      !is_declarative_performance_observer_disabled_for_document_) {
    auto lcp_mojom_entry = mojom::blink::DeclarativePerformanceEntry::NewLcp(
        mojom::blink::DeclarativeLargestContentfulPaint::New(
            base::Milliseconds(entry->startTime()), entry->size(),
            base::Milliseconds(entry->renderTime()),
            base::Milliseconds(entry->loadTime()), entry->id(), entry->url(),
            entry->element() ? entry->element()->tagName() : String()));
    BufferPerformanceEntry(std::move(lcp_mojom_entry));
  }
}

void Performance::AddInteractionContentfulPaint(
    InteractionContentfulPaint* entry) {
  probe::PerformanceEntryAdded(GetExecutionContext(), entry);
  if (interaction_contentful_paint_buffer_.size() <
      kDefaultInteractionContenfulPaintSize) {
    InsertEntryIntoSortedBuffer(interaction_contentful_paint_buffer_, *entry);
  } else {
    ++(dropped_entries_count_map_
           .find(PerformanceEntry::kInteractionContentfulPaint)
           ->value);
  }
}

void Performance::AddSoftNavigationToPerformanceTimeline(
    PerformanceSoftNavigation* entry) {
  probe::PerformanceEntryAdded(GetExecutionContext(), entry);
  if (soft_navigation_buffer_.size() < kDefaultSoftNavigationBufferSize) {
    InsertEntryIntoSortedBuffer(soft_navigation_buffer_, *entry);
  } else {
    ++(dropped_entries_count_map_.find(PerformanceEntry::kSoftNavigation)
           ->value);
  }
}

bool Performance::CanAddResourceTimingEntry() {
  // https://w3c.github.io/resource-timing/#dfn-can-add-resource-timing-entry
  return resource_timing_buffer_.size() < resource_timing_buffer_size_limit_;
}

void Performance::AddLongTaskTiming(base::TimeTicks start_time,
                                    base::TimeTicks end_time,
                                    const AtomicString& name,
                                    const AtomicString& container_type,
                                    const AtomicString& container_src,
                                    const AtomicString& container_id,
                                    const AtomicString& container_name) {
  double dom_high_res_start_time =
      MonotonicTimeToDOMHighResTimeStamp(start_time);

  ExecutionContext* execution_context = GetExecutionContext();
  auto* entry = MakeGarbageCollected<PerformanceLongTaskTiming>(
      dom_high_res_start_time,
      // Convert the delta between start and end times to an int to reduce the
      // granularity of the duration to 1 ms.
      static_cast<int>(MonotonicTimeToDOMHighResTimeStamp(end_time) -
                       dom_high_res_start_time),
      name, container_type, container_src, container_id, container_name,
      DynamicTo<LocalDOMWindow>(execution_context),
      NavigationId().web_exposed_id);
  if (longtask_buffer_.size() < kDefaultLongTaskBufferSize) {
    InsertEntryIntoSortedBuffer(longtask_buffer_, *entry);
  } else {
    ++(dropped_entries_count_map_.find(PerformanceEntry::kLongTask)->value);
    UseCounter::Count(execution_context, WebFeature::kLongTaskBufferFull);
  }
  if ((++long_task_counter_ % kLongTaskUkmSampleInterval) == 0) {
    RecordLongTaskUkm(execution_context,
                      base::Milliseconds(dom_high_res_start_time),
                      end_time - start_time);
  }
  NotifyObserversOfEntry(*entry);
}

void Performance::AddBackForwardCacheRestoration(
    base::TimeTicks start_time,
    base::TimeTicks pageshow_start_time,
    base::TimeTicks pageshow_end_time) {
  auto* entry = MakeGarbageCollected<BackForwardCacheRestoration>(
      MonotonicTimeToDOMHighResTimeStamp(start_time),
      MonotonicTimeToDOMHighResTimeStamp(pageshow_start_time),
      MonotonicTimeToDOMHighResTimeStamp(pageshow_end_time),
      DynamicTo<LocalDOMWindow>(GetExecutionContext()),
      NavigationId().web_exposed_id);
  if (back_forward_cache_restoration_buffer_.size() <
      back_forward_cache_restoration_buffer_size_limit_) {
    InsertEntryIntoSortedBuffer(back_forward_cache_restoration_buffer_, *entry);
  } else {
    ++(dropped_entries_count_map_
           .find(PerformanceEntry::kBackForwardCacheRestoration)
           ->value);
  }
  NotifyObserversOfEntry(*entry);
}

UserTiming& Performance::GetUserTiming() {
  if (!user_timing_)
    user_timing_ = MakeGarbageCollected<UserTiming>(*this);
  return *user_timing_;
}

void Performance::clearMarks(const AtomicString& mark_name) {
  GetUserTiming().ClearMarks(mark_name);
}

void Performance::clearMeasures(const AtomicString& measure_name) {
  GetUserTiming().ClearMeasures(measure_name);
}

// These three used to walk observers_/active_observers_/suspended_observers_
// (HeapLinkedHashSet<Member<PerformanceObserver>>) to fan an entry out to
// registered PerformanceObservers. PerformanceObserver itself -- along with
// the V8 callback and the RegisterPerformanceObserver()/
// UnregisterPerformanceObserver() entry points that populated those sets --
// was deleted in the v8ectomy pass, so those member sets no longer exist and
// nothing can ever register an observer. WindowPerformance still calls these
// (protected) methods when it adds entries to the timeline, so the methods
// stay, honestly doing nothing: an empty observer set has nothing to notify.
void Performance::NotifyObserversOfEntry(PerformanceEntry&) const {}

void Performance::NotifyObserversOfContainerEntry(PerformanceEntry& entry) const {
  CHECK(entry.EntryTypeEnum() == PerformanceEntry::kContainer);
}

void Performance::NotifyObserversOfContainerTiming() {}

bool Performance::HasObserverFor(
    PerformanceEntry::EntryType filter_type) const {
  return observer_filter_options_ & filter_type;
}

void Performance::DeliverObservationsTimerFired(TimerBase*) {
  if (HasObserverFor(PerformanceEntry::kContainer)) {
    PopulateContainerTimingEntries();
  }
  // Used to swap out active_observers_ (a HeapLinkedHashSet<Member<
  // PerformanceObserver>>) and call observer->Deliver() on each one.
  // PerformanceObserver no longer exists (see the comment above
  // NotifyObserversOfEntry()), so there is nothing left to swap or deliver
  // to.
}

int Performance::GetDroppedEntriesForTypes(PerformanceEntryTypeMask types) {
  int dropped_count = 0;
  for (const auto type : kDroppableEntryTypes) {
    if (types & type)
      dropped_count += dropped_entries_count_map_.at(type);
  }
  return dropped_count;
}

// static
DOMHighResTimeStamp Performance::ClampTimeResolution(
    base::TimeDelta time,
    bool cross_origin_isolated_capability) {
  static TimeClamper clamper;
  return clamper.ClampTimeResolution(time, cross_origin_isolated_capability)
      .InMillisecondsF();
}

// static
DOMHighResTimeStamp Performance::MonotonicTimeToDOMHighResTimeStamp(
    base::TimeTicks time_origin,
    base::TimeTicks monotonic_time,
    bool allow_negative_value,
    bool cross_origin_isolated_capability) {
  // Avoid exposing raw platform timestamps.
  if (monotonic_time.is_null() || time_origin.is_null())
    return 0.0;

  DOMHighResTimeStamp clamped_time =
      ClampTimeResolution(monotonic_time.since_origin(),
                          cross_origin_isolated_capability) -
      ClampTimeResolution(time_origin.since_origin(),
                          cross_origin_isolated_capability);
  if (clamped_time < 0 && !allow_negative_value)
    return 0.0;
  return clamped_time;
}

DOMHighResTimeStamp Performance::MonotonicTimeToDOMHighResTimeStamp(
    base::TimeTicks monotonic_time) const {
  return MonotonicTimeToDOMHighResTimeStamp(time_origin_, monotonic_time,
                                            false /* allow_negative_value */,
                                            cross_origin_isolated_capability_);
}

DOMHighResTimeStamp Performance::now() const {
  return MonotonicTimeToDOMHighResTimeStamp(base::TimeTicks::Now());
}

// static
bool Performance::CanExposeNode(Node* node) {
  if (!node || !node->isConnected() || node->IsInShadowTree())
    return false;

  // Do not expose |node| when the document is not 'fully active'.
  const Document& document = node->GetDocument();
  if (!document.IsActive() || !document.GetFrame())
    return false;

  return true;
}

void Performance::AddPaintTiming(PerformancePaintTiming::PaintType type,
                                 const DOMPaintTimingInfo& paint_timing_info) {
  PerformancePaintTiming* entry = MakeGarbageCollected<PerformancePaintTiming>(
      type, paint_timing_info, DynamicTo<LocalDOMWindow>(GetExecutionContext()),
      NavigationId().web_exposed_id);
  DCHECK((type == PerformancePaintTiming::PaintType::kFirstPaint) ||
         (type == PerformancePaintTiming::PaintType::kFirstContentfulPaint));

  if (paint_entries_timing_.size() < kDefaultPaintEntriesBufferSize) {
    InsertEntryIntoSortedBuffer(paint_entries_timing_, *entry);
  } else {
    ++(dropped_entries_count_map_.find(PerformanceEntry::kPaint)->value);
  }
  NotifyObserversOfEntry(*entry);
}

// Insert entry in PerformanceEntryVector while maintaining sorted order (via
// Bubble Sort). We assume that the order of insertion roughly corresponds to
// the order of the StartTime, hence the sort beginning from the tail-end.
void Performance::InsertEntryIntoSortedBuffer(PerformanceEntryVector& entries,
                                              PerformanceEntry& entry) {
  entries.push_back(&entry);

  if (entries.size() > 1) {
    // Bubble Sort from tail.
    int left = entries.size() - 2;
    while (left >= 0 &&
           entries[left]->startTime() > entries[left + 1]->startTime()) {
      SwapEntries(entries, left, left + 1);
      left--;
    }
  }

  return;
}

void Performance::Trace(Visitor* visitor) const {
  visitor->Trace(resource_timing_buffer_);
  visitor->Trace(resource_timing_secondary_buffer_);
  visitor->Trace(container_timing_buffer_);
  visitor->Trace(element_timing_buffer_);
  visitor->Trace(scroll_timing_buffer_);
  visitor->Trace(event_timing_buffer_);
  visitor->Trace(layout_shift_buffer_);
  visitor->Trace(largest_contentful_paint_buffer_);
  visitor->Trace(interaction_contentful_paint_buffer_);
  visitor->Trace(longtask_buffer_);
  visitor->Trace(visibility_state_buffer_);
  visitor->Trace(back_forward_cache_restoration_buffer_);
  visitor->Trace(soft_navigation_buffer_);
  visitor->Trace(long_animation_frame_buffer_);
  visitor->Trace(navigation_timing_);
  visitor->Trace(user_timing_);
  visitor->Trace(paint_entries_timing_);
  visitor->Trace(first_input_timing_);
  // observers_, active_observers_ and suspended_observers_ used to be traced
  // here; they no longer exist now that PerformanceObserver is gone (see the
  // comment above NotifyObserversOfEntry()).
  visitor->Trace(deliver_observations_timer_);
  visitor->Trace(resource_timing_buffer_full_timer_);
  visitor->Trace(background_tracing_helper_);
  visitor->Trace(performance_entries_flush_timer_);
  visitor->Trace(declarative_performance_observer_host_);
  EventTarget::Trace(visitor);
}

void Performance::BufferPerformanceEntry(
    mojom::blink::DeclarativePerformanceEntryPtr entry) {
  batched_performance_entries_.push_back(std::move(entry));
  if (!performance_entries_flush_timer_.IsActive()) {
    performance_entries_flush_timer_.StartOneShot(kBufferTimerDelay, FROM_HERE);
  }
}

void Performance::PerformanceEntriesFlushTimerFired(TimerBase*) {
  FlushPerformanceEntries();
}

void Performance::FlushPerformanceEntries() {
  performance_entries_flush_timer_.Stop();
  if (batched_performance_entries_.empty()) {
    return;
  }

  if (ExecutionContext* execution_context = GetExecutionContext()) {
    if (LocalDOMWindow* window = DynamicTo<LocalDOMWindow>(execution_context)) {
      if (LocalFrame* frame = window->GetFrame()) {
        if (!declarative_performance_observer_host_.is_bound()) {
          frame->GetBrowserInterfaceBroker().GetInterface(
              declarative_performance_observer_host_.BindNewPipeAndPassReceiver(
                  frame->GetTaskRunner(TaskType::kInternalDefault)));
          declarative_performance_observer_host_.set_disconnect_handler(
              BindOnce(&Performance::
                           OnDeclarativePerformanceObserverHostDisconnected,
                       WrapWeakPersistent(this)));
        }
        declarative_performance_observer_host_->DidObservePerformanceEntries(
            std::move(batched_performance_entries_));
      }
    }
  }
  batched_performance_entries_.clear();
}

void Performance::ResetTimeOriginForTesting(base::TimeTicks time_origin) {
  time_origin_ = time_origin;
}

void Performance::OnDeclarativePerformanceObserverHostDisconnected() {
  is_declarative_performance_observer_disabled_for_document_ = true;
  declarative_performance_observer_host_.reset();
  batched_performance_entries_.clear();
}

}  // namespace blink
