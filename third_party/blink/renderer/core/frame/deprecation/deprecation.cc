// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/deprecation/deprecation.h"

#include "base/command_line.h"
#include "build/build_config.h"
#include "third_party/blink/public/common/switches.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink.h"
#include "third_party/blink/public/mojom/reporting/reporting.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/deprecation/deprecation_info.h"
#include "third_party/blink/renderer/core/frame/deprecation/deprecation_report_body.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/report.h"
#include "third_party/blink/renderer/core/frame/reporting_context.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/origin_trials/origin_trial_context.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

namespace {

// Send the deprecation info to the browser process, currently only supports
// frame.
void SendToBrowser(ExecutionContext* context, const DeprecationInfo& info) {
  // Command line switch is set when the feature is turned on by the browser
  // process.
  if (!base::CommandLine::ForCurrentProcess()->HasSwitch(
          blink::switches::kLegacyTechReportPolicyEnabled)) {
    return;
  }

  if (auto* window = DynamicTo<LocalDOMWindow>(context)) {
    if (LocalFrame* frame = window->GetFrame()) {
      // CaptureSourceLocation(ExecutionContext*) used to capture the current
      // JS call stack in that context's V8 isolate. There is no JS stack to
      // capture any more; the no-argument overload always returns an
      // unknown location.
      SourceLocation* source_location = CaptureSourceLocation();
      frame->GetLocalFrameHostRemote().SendLegacyTechEvent(
          info.type_.ToString(),
          mojom::blink::LegacyTechEventCodeLocation::New(
              source_location->Url() ? source_location->Url() : g_empty_string,
              source_location->LineNumber(), source_location->ColumnNumber()));
    }
  }
}

}  // namespace

Deprecation::Deprecation() : mute_count_(0) {}

void Deprecation::ClearSuppression() {
  features_deprecation_bits_.reset();
}

void Deprecation::MuteForInspector() {
  mute_count_++;
}

void Deprecation::UnmuteForInspector() {
  mute_count_--;
}

void Deprecation::SetReported(WebFeature feature) {
  features_deprecation_bits_.set(static_cast<size_t>(feature));
}

bool Deprecation::GetReported(WebFeature feature) const {
  return features_deprecation_bits_[static_cast<size_t>(feature)];
}

void Deprecation::CountDeprecationCrossOriginIframe(LocalDOMWindow* window,
                                                    WebFeature feature) {
  DCHECK(window);
  if (!window->GetFrame())
    return;

  // Check to see if the frame can script into the top level context.
  Frame& top = window->GetFrame()->Tree().Top();
  if (!window->GetSecurityOrigin()->CanAccess(
          top.GetSecurityContext()->GetSecurityOrigin())) {
    CountDeprecation(window, feature);
  }
}

void Deprecation::CountDeprecation(ExecutionContext* context,
                                   WebFeature feature) {
  if (!context)
    return;

  // Worker and worklet global scopes used to keep their own Deprecation via
  // WorkerOrWorkletGlobalScope::GetDeprecation() here. Workers require a
  // script engine to run, which this renderer no longer has, so that global
  // scope type is gone; every ExecutionContext reaching this point is a
  // Document's LocalDOMWindow.
  Deprecation* deprecation = nullptr;
  if (auto* window = DynamicTo<LocalDOMWindow>(context)) {
    if (window->GetFrame())
      deprecation = &window->GetFrame()->GetPage()->GetDeprecation();
  }

  if (!deprecation || deprecation->mute_count_ ||
      deprecation->GetReported(feature)) {
    return;
  }
  deprecation->SetReported(feature);
  context->CountUse(feature);
  const DeprecationInfo info = GetDeprecationInfo(feature);

  String type = info.type_.ToString();
  // Send the deprecation message as a DevTools issue.
  // AuditsIssue::ReportDeprecationIssue(...) was here.

  // Send the deprecation message to browser process for enterprise usage.
  SendToBrowser(context, info);

  // Send the deprecation report to the Reporting API and any
  // ReportingObservers.
  DeprecationReportBody* body = MakeGarbageCollected<DeprecationReportBody>(
      type, std::nullopt, info.message_.ToString());
  Report* report = MakeGarbageCollected<Report>(ReportType::kDeprecation,
                                                context->Url(), body);
  ReportingContext::From(context)->QueueReport(report);
}

// static
bool Deprecation::IsDeprecated(WebFeature feature) {
  return GetDeprecationInfo(feature).type_ != kNotDeprecated;
}

}  // namespace blink
