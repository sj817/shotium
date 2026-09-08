// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/reporting_context.h"

#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/csp/csp_violation_report_body.h"
#include "third_party/blink/renderer/core/frame/deprecation/deprecation_report_body.h"
#include "third_party/blink/renderer/core/frame/document_policy_violation_report_body.h"
#include "third_party/blink/renderer/core/frame/integrity_violation_report_body.h"
#include "third_party/blink/renderer/core/frame/intervention_report_body.h"
#include "third_party/blink/renderer/core/frame/permissions_policy_violation_report_body.h"
#include "third_party/blink/renderer/core/frame/report.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"

namespace blink {


// static
const char ReportingContext::kSupplementName[] = "ReportingContext";

ReportingContext::ReportingContext(ExecutionContext& context)
    : Supplement<ExecutionContext>(context),
      execution_context_(context),
      reporting_service_(&context) {}

// static
ReportingContext* ReportingContext::From(ExecutionContext* context) {
  ReportingContext* reporting_context =
      Supplement<ExecutionContext>::From<ReportingContext>(context);
  if (!reporting_context) {
    reporting_context = MakeGarbageCollected<ReportingContext>(*context);
    Supplement<ExecutionContext>::ProvideTo(*context, reporting_context);
  }
  return reporting_context;
}

void ReportingContext::QueueReport(Report* report,
                                   const Vector<String>& endpoints) {
  if (!report->ShouldSendReport()) {
    return;
  }

  CountReport(report);

  // Send the report via the Reporting API.
  for (auto& endpoint : endpoints)
    SendToReportingAPI(report, endpoint);
}

void ReportingContext::Trace(Visitor* visitor) const {
  visitor->Trace(execution_context_);
  visitor->Trace(reporting_service_);
  Supplement<ExecutionContext>::Trace(visitor);
}

void ReportingContext::CountReport(Report* report) {
  const String& type = report->type();
  WebFeature feature;

  if (type == ReportType::kDeprecation) {
    feature = WebFeature::kDeprecationReport;
  } else if (type == ReportType::kPermissionsPolicyViolation ||
             type == ReportType::kPotentialPermissionsPolicyViolation) {
    feature = WebFeature::kFeaturePolicyReport;
  } else if (type == ReportType::kIntervention) {
    feature = WebFeature::kInterventionReport;
  } else {
    return;
  }

  UseCounter::Count(execution_context_, feature);
}

const HeapMojoRemote<mojom::blink::ReportingServiceProxy>&
ReportingContext::GetReportingService() const {
  if (!reporting_service_.is_bound()) {
    execution_context_->GetBrowserInterfaceBroker().GetInterface(
        reporting_service_.BindNewPipeAndPassReceiver(
            execution_context_->GetTaskRunner(TaskType::kMiscPlatformAPI)));
  }
  return reporting_service_;
}

void ReportingContext::SendToReportingAPI(Report* report,
                                          const String& endpoint) const {
  const String& type = report->type();
  // ReportType::kCSPHash is no longer produced: CSPHashReportBody was
  // deleted along with ContentSecurityPolicy::AddHashReportIfNeeded()'s
  // report-queueing body (a screenshot engine never reports CSP
  // violations), so it is intentionally absent from this list.
  if (!(type == ReportType::kCSPViolation ||
        type == ReportType::kDeprecation ||
        type == ReportType::kPermissionsPolicyViolation ||
        type == ReportType::kPotentialPermissionsPolicyViolation ||
        type == ReportType::kIntegrityViolation ||
        type == ReportType::kIntervention ||
        type == ReportType::kDocumentPolicyViolation)) {
    return;
  }

  KURL url = KURL(report->url());
  // CSP Hash and IntegrityPolicy reports are not a LocationReportBody.
  // CSPHashReportBody was deleted: a screenshot engine never triggers a CSP
  // subresource-integrity hash mismatch report (see
  // ContentSecurityPolicy::AddHashReportIfNeeded()), so ReportType::kCSPHash
  // can no longer occur here.
  if (type == ReportType::kIntegrityViolation) {
    const IntegrityViolationReportBody* body =
        static_cast<IntegrityViolationReportBody*>(report->body());
    GetReportingService()->QueueIntegrityViolationReport(
        url, endpoint, body->documentURL(), body->blockedURL(),
        body->destination(), body->reportOnly());
    return;
  }

  const LocationReportBody* location_body =
      static_cast<LocationReportBody*>(report->body());
  int line_number = location_body->lineNumber().value_or(0);
  int column_number = location_body->columnNumber().value_or(0);

  if (type == ReportType::kCSPViolation) {
    // Send the CSP violation report.
    const CSPViolationReportBody* body =
        static_cast<CSPViolationReportBody*>(report->body());
    GetReportingService()->QueueCspViolationReport(
        url, endpoint, body->documentURL() ? body->documentURL() : "",
        body->referrer(), body->blockedURL(),
        body->effectiveDirective() ? body->effectiveDirective() : "",
        body->originalPolicy() ? body->originalPolicy() : "",
        body->sourceFile(), body->sample(), body->disposition().AsString(),
        body->statusCode(), line_number, column_number, body->urlHash(),
        body->evalHash());
  } else if (type == ReportType::kDeprecation) {
    // Send the deprecation report.
    const DeprecationReportBody* body =
        static_cast<DeprecationReportBody*>(report->body());
    GetReportingService()->QueueDeprecationReport(
        url, body->id(), body->AnticipatedRemoval(),
        body->message().IsNull() ? g_empty_string : body->message(),
        body->sourceFile(), line_number, column_number);
  } else if (type == ReportType::kPermissionsPolicyViolation) {
    // Send the permissions policy violation report.
    const PermissionsPolicyViolationReportBody* body =
        static_cast<PermissionsPolicyViolationReportBody*>(report->body());
    GetReportingService()->QueuePermissionsPolicyViolationReport(
        url, endpoint, body->featureId(), body->disposition(), body->message(),
        body->sourceFile(), line_number, column_number);
  } else if (type == ReportType::kPotentialPermissionsPolicyViolation) {
    // Send the potential permissions policy violation report.
    const PermissionsPolicyViolationReportBody* body =
        static_cast<PermissionsPolicyViolationReportBody*>(report->body());
    GetReportingService()->QueuePotentialPermissionsPolicyViolationReport(
        url, endpoint, body->featureId(), body->disposition(), body->message(),
        body->allowAttribute(), body->srcAttribute(), body->sourceFile(),
        line_number, column_number);
  } else if (type == ReportType::kIntervention) {
    // Send the intervention report.
    const InterventionReportBody* body =
        static_cast<InterventionReportBody*>(report->body());
    GetReportingService()->QueueInterventionReport(
        url, body->id(),
        body->message().IsNull() ? g_empty_string : body->message(),
        body->sourceFile(), line_number, column_number);
  } else if (type == ReportType::kDocumentPolicyViolation) {
    const DocumentPolicyViolationReportBody* body =
        static_cast<DocumentPolicyViolationReportBody*>(report->body());
    // Send the document policy violation report.
    GetReportingService()->QueueDocumentPolicyViolationReport(
        url, endpoint, body->featureId(), body->disposition(), body->message(),
        body->sourceFile(), line_number, column_number);
  }
}

}  // namespace blink
