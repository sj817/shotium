// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/metrics/public/cpp/ukm_source.h"

#include <string>
#include <utility>
#include <vector>

#include "base/atomicops.h"
#include "base/check_op.h"
#include "base/hash/hash.h"
#include "base/notreached.h"
#include "services/metrics/public/cpp/ukm_source_id.h"

namespace ukm {

namespace {

int32_t g_android_activity_type_state = -1;

}  // namespace

// static
void UkmSource::SetAndroidActivityTypeState(int32_t activity_type) {
  g_android_activity_type_state = activity_type;
}

UkmSource::NavigationData::NavigationData() = default;
UkmSource::NavigationData::~NavigationData() = default;

UkmSource::NavigationData::NavigationData(const NavigationData& other) =
    default;

UkmSource::NavigationData UkmSource::NavigationData::CopyWithSanitizedUrls(
    std::vector<GURL> sanitized_urls) const {
  DCHECK(!sanitized_urls.empty());
  DCHECK(!sanitized_urls.back().is_empty());
  DCHECK(!sanitized_urls.front().is_empty());

  NavigationData sanitized_navigation_data;
  sanitized_navigation_data.urls = std::move(sanitized_urls);
  sanitized_navigation_data.previous_source_id = previous_source_id;
  sanitized_navigation_data.previous_same_document_source_id =
      previous_same_document_source_id;
  sanitized_navigation_data.opener_source_id = opener_source_id;
  sanitized_navigation_data.tab_id = tab_id;
  sanitized_navigation_data.is_same_document_navigation =
      is_same_document_navigation;
  sanitized_navigation_data.same_origin_status = same_origin_status;
  sanitized_navigation_data.is_renderer_initiated = is_renderer_initiated;
  sanitized_navigation_data.is_error_page = is_error_page;
  sanitized_navigation_data.navigation_time = navigation_time;
  sanitized_navigation_data.resolved_urls = resolved_urls;
  return sanitized_navigation_data;
}

UkmSource::UkmSource(ukm::SourceId id, const GURL& url)
    : id_(id),
      type_(GetSourceIdType(id_)),
      android_activity_type_state_(g_android_activity_type_state),
      creation_time_(base::TimeTicks::Now()) {
  if (!url.is_empty()) {
    navigation_data_.urls = {url};
  }
}

UkmSource::UkmSource(ukm::SourceId id, const NavigationData& navigation_data)
    : id_(id),
      type_(GetSourceIdType(id_)),
      navigation_data_(navigation_data),
      android_activity_type_state_(g_android_activity_type_state),
      creation_time_(base::TimeTicks::Now()) {
  DCHECK(type_ == SourceIdType::NAVIGATION_ID);
}

UkmSource::~UkmSource() = default;

void UkmSource::UpdateUrl(const GURL& new_url) {
  DCHECK(!new_url.is_empty());
  DCHECK_EQ(1u, navigation_data_.urls.size());
  if (url() == new_url)
    return;
  navigation_data_.urls = {new_url};
}

void UkmSource::set_resolved_urls(const std::vector<GURL>& resolved_urls) {
  navigation_data_.resolved_urls = resolved_urls;
}

}  // namespace ukm
