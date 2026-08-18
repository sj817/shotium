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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_MARK_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_MARK_H_

#include "base/time/time.h"
#include "third_party/blink/public/mojom/timing/performance_mark_or_measure.mojom-blink-forward.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/timing/performance_entry.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_map.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ExceptionState;
class PerformanceMarkOptions;

class CORE_EXPORT PerformanceMark final : public PerformanceEntry {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // This constructor is only public so that MakeGarbageCollected can call it.
  PerformanceMark(const AtomicString& name,
                  double start_time,
                  base::TimeTicks unsafe_time_for_traces,
                  ExceptionState& exception_state,
                  DOMWindow* source,
                  uint64_t navigation_id);

  ~PerformanceMark() override = default;

  const AtomicString& entryType() const override;
  PerformanceEntryType EntryTypeEnum() const override;
  mojom::blink::PerformanceMarkOrMeasurePtr ToMojoPerformanceMarkOrMeasure()
      override;

  void Trace(Visitor*) const override;

  base::TimeTicks UnsafeTimeForTraces() const {
    return unsafe_time_for_traces_;
  }

  using UserFeatureNameToWebFeatureMap =
      HashMap<String, mojom::blink::WebFeature>;
  static const UserFeatureNameToWebFeatureMap&
  GetUseCounterMappingForTesting() {
    return GetUseCounterMapping();
  }
  static std::optional<mojom::blink::WebFeature>
  GetWebFeatureForUserFeatureName(const String& feature_name);

 private:
  static const UserFeatureNameToWebFeatureMap& GetUseCounterMapping();
  base::TimeTicks unsafe_time_for_traces_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TIMING_PERFORMANCE_MARK_H_
