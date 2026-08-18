// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/coop_access_violation_report_body.h"
#include "services/network/public/cpp/cross_origin_opener_policy.h"

namespace blink {

CoopAccessViolationReportBody::CoopAccessViolationReportBody(
    SourceLocation* source_location,
    network::mojom::blink::CoopAccessReportType type,
    const String& property,
    const String& reported_url)
    : LocationReportBody(source_location),
      type_(type),
      property_(property),
      reported_url_(reported_url) {}

String CoopAccessViolationReportBody::type() const {
  return network::CoopAccessReportTypeToString(type_);
}

String CoopAccessViolationReportBody::openeeURL() const {
  switch (type_) {
    case network::mojom::CoopAccessReportType::kAccessFromCoopPageToOpenee:
      return reported_url_;

    case network::mojom::CoopAccessReportType::kAccessToCoopPageFromOpener:
    case network::mojom::CoopAccessReportType::kAccessToCoopPageFromOpenee:
    case network::mojom::CoopAccessReportType::kAccessToCoopPageFromOther:
      NOTREACHED();
    case network::mojom::CoopAccessReportType::kAccessFromCoopPageToOpener:
    case network::mojom::CoopAccessReportType::kAccessFromCoopPageToOther:
      return String();
  }
}

String CoopAccessViolationReportBody::openerURL() const {
  switch (type_) {
    case network::mojom::CoopAccessReportType::kAccessFromCoopPageToOpener:
      return reported_url_;

    case network::mojom::CoopAccessReportType::kAccessToCoopPageFromOpener:
    case network::mojom::CoopAccessReportType::kAccessToCoopPageFromOpenee:
    case network::mojom::CoopAccessReportType::kAccessToCoopPageFromOther:
      NOTREACHED();
    case network::mojom::CoopAccessReportType::kAccessFromCoopPageToOpenee:
    case network::mojom::CoopAccessReportType::kAccessFromCoopPageToOther:
      return String();
  }
}

String CoopAccessViolationReportBody::otherDocumentURL() const {
  switch (type_) {
    case network::mojom::CoopAccessReportType::kAccessFromCoopPageToOther:
      return reported_url_;

    case network::mojom::CoopAccessReportType::kAccessToCoopPageFromOpener:
    case network::mojom::CoopAccessReportType::kAccessToCoopPageFromOpenee:
    case network::mojom::CoopAccessReportType::kAccessToCoopPageFromOther:
      NOTREACHED();
    case network::mojom::CoopAccessReportType::kAccessFromCoopPageToOpenee:
    case network::mojom::CoopAccessReportType::kAccessFromCoopPageToOpener:
      return String();
  }
}


}  // namespace blink
