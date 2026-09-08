// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#include "third_party/blink/renderer/core/timing/performance_mark.h"

#include <optional>

#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/performance_entry_names.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/performance.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/trace_event.h"

namespace blink {

const AtomicString& PerformanceMark::entryType() const {
  return performance_entry_names::kMark;
}

PerformanceEntryType PerformanceMark::EntryTypeEnum() const {
  return PerformanceEntry::EntryType::kMark;
}

// static
const PerformanceMark::UserFeatureNameToWebFeatureMap&
PerformanceMark::GetUseCounterMapping() {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(
      ThreadSpecific<UserFeatureNameToWebFeatureMap>, map, ());
  if (!map.IsSet()) {
    *map = {
        {"NgOptimizedImage", WebFeature::kUserFeatureNgOptimizedImage},
        {"NgAfterRender", WebFeature::kUserFeatureNgAfterRender},
        {"NgHydration", WebFeature::kUserFeatureNgHydration},
        {"next-third-parties-ga", WebFeature::kUserFeatureNextThirdPartiesGA},
        {"next-third-parties-gtm", WebFeature::kUserFeatureNextThirdPartiesGTM},
        {"next-third-parties-YouTubeEmbed",
         WebFeature::kUserFeatureNextThirdPartiesYouTubeEmbed},
        {"next-third-parties-GoogleMapsEmbed",
         WebFeature::kUserFeatureNextThirdPartiesGoogleMapsEmbed},
        {"nuxt-image", WebFeature::kUserFeatureNuxtImage},
        {"nuxt-picture", WebFeature::kUserFeatureNuxtPicture},
        {"nuxt-third-parties-ga", WebFeature::kUserFeatureNuxtThirdPartiesGA},
        {"nuxt-third-parties-gtm", WebFeature::kUserFeatureNuxtThirdPartiesGTM},
        {"nuxt-third-parties-YouTubeEmbed",
         WebFeature::kUserFeatureNuxtThirdPartiesYouTubeEmbed},
        {"nuxt-third-parties-GoogleMaps",
         WebFeature::kUserFeatureNuxtThirdPartiesGoogleMaps},
    };
  }
  return *map;
}

// static
std::optional<mojom::blink::WebFeature>
PerformanceMark::GetWebFeatureForUserFeatureName(const String& feature_name) {
  auto& feature_map = PerformanceMark::GetUseCounterMapping();
  auto it = feature_map.find(feature_name);
  if (it == feature_map.end()) {
    return std::nullopt;
  }

  return it->value;
}

void PerformanceMark::Trace(Visitor* visitor) const {
  PerformanceEntry::Trace(visitor);
}

}  // namespace blink
