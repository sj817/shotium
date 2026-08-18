// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/resource_load_observer_for_frame.h"

#include <optional>

#include "base/numerics/safe_conversions.h"
#include "base/types/optional_util.h"
#include "services/network/public/cpp/cors/cors_error_status.h"
#include "services/network/public/mojom/cors.mojom-forward.h"
#include "third_party/blink/public/common/security/address_space_feature.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink.h"
#include "third_party/blink/renderer/core/core_probes_inl.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/execution_context/agent.h"
#include "third_party/blink/renderer/core/frame/deprecation/deprecation.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/loader/idleness_detector.h"
#include "third_party/blink/renderer/core/loader/interactive_detector.h"
#include "third_party/blink/renderer/core/loader/mixed_content_checker.h"
#include "third_party/blink/renderer/core/loader/preload_helper.h"
#include "third_party/blink/renderer/core/loader/progress_tracker.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_client_settings_object.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_context.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_info.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_parameters.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_error.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher_properties.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_request.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"

namespace blink {
namespace {

// The list of features which should be reported as deprecated.
constexpr WebFeature kDeprecatedAddressSpaceFeatures[] = {
    WebFeature::kAddressSpacePublicNonSecureContextEmbeddedLocalV2,
    WebFeature::kAddressSpacePublicNonSecureContextEmbeddedLoopbackV2,
    WebFeature::kAddressSpaceLocalNonSecureContextEmbeddedLoopbackV2,
};

// Returns whether |feature| is deprecated.
bool IsDeprecatedAddressSpaceFeature(WebFeature feature) {
  for (WebFeature entry : kDeprecatedAddressSpaceFeatures) {
    if (feature == entry) {
      return true;
    }
  }
  return false;
}

// Increments the correct kAddressSpace* WebFeature UseCounter corresponding to
// the given |client_frame| performing a subresource fetch |fetch_type| and
// receiving the given |response|.
//
// Does nothing if |client_frame| is nullptr.
void RecordAddressSpaceFeature(LocalFrame* client_frame,
                               const ResourceResponse& response) {
  if (!client_frame) {
    return;
  }

  LocalDOMWindow* window = client_frame->DomWindow();

  if (response.RemoteIPEndpoint().address().IsZero()) {
    UseCounter::Count(window, WebFeature::kPrivateNetworkAccessNullIpAddress);
  }

  std::optional<WebFeature> feature = AddressSpaceFeature(
      FetchType::kSubresource, response.ClientAddressSpace(),
      window->IsSecureContext(), response.AddressSpace());
  if (!feature.has_value()) {
    return;
  }

  // This WebFeature encompasses all private network requests.
  UseCounter::Count(window,
                    WebFeature::kMixedContentPrivateHostnameInPublicHostname);

  if (IsDeprecatedAddressSpaceFeature(*feature)) {
    Deprecation::CountDeprecation(window, *feature);
  } else {
    UseCounter::Count(window, *feature);
  }
}

}  // namespace

ResourceLoadObserverForFrame::ResourceLoadObserverForFrame(
    DocumentLoader& loader,
    Document& document,
    const ResourceFetcherProperties& fetcher_properties)
    : document_loader_(loader),
      document_(document),
      fetcher_properties_(fetcher_properties) {}
ResourceLoadObserverForFrame::~ResourceLoadObserverForFrame() = default;

void ResourceLoadObserverForFrame::DidStartRequest(
    const FetchParameters& params,
    ResourceType resource_type) {
  // This hook existed only to feed the DOM activity logger, which reported
  // resource requests to isolated-world extension code. There is no script
  // engine and no isolated worlds, so there is nothing left to observe here.
}

void ResourceLoadObserverForFrame::WillSendRequest(
    const ResourceRequest& request,
    const ResourceResponse& redirect_response,
    ResourceType resource_type,
    const ResourceLoaderOptions& options,
    RenderBlockingBehavior render_blocking_behavior,
    const Resource* resource) {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  if (redirect_response.IsNull()) {
    // Progress doesn't care about redirects, only notify it when an
    // initial request is sent.
    frame->Loader().Progress().WillStartLoading(request.InspectorId(),
                                                request.Priority());
  }

  probe::WillSendRequest(
      document_->domWindow(), document_loader_,
      fetcher_properties_->GetFetchClientSettingsObject().GlobalObjectUrl(),
      request, redirect_response, options, resource_type,
      render_blocking_behavior, base::TimeTicks::Now());
  if (auto* idleness_detector = frame->GetIdlenessDetector())
    idleness_detector->OnWillSendRequest(document_->Fetcher());
  if (auto* interactive_detector = InteractiveDetector::From(*document_))
    interactive_detector->OnResourceLoadBegin(std::nullopt);
}

void ResourceLoadObserverForFrame::DidChangePriority(
    uint64_t identifier,
    ResourceLoadPriority priority,
    int intra_priority_value) {
  // DEVTOOLS_TIMELINE_TRACE_EVENT(...) was here.
  probe::DidChangeResourcePriority(document_->GetFrame(), document_loader_,
                                   identifier, priority);
}

void ResourceLoadObserverForFrame::DidReceiveResponse(
    uint64_t identifier,
    const ResourceRequest& request,
    const ResourceResponse& response,
    const Resource* resource,
    ResponseSource response_source) {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);

  // Track resource metrics by identifier for byte tracking in DidReceiveData.
  // Only track resources that actually have responses and will send data.
  if (!guardrails_policy_state_initialized_) {
    guardrails_policy_state_ =
        document_->GetExecutionContext()->GetGuardrailsPolicyState();
    guardrails_policy_state_initialized_ = true;
  }
  if (guardrails_policy_state_.has_value() && resource &&
      resource->GetType() == ResourceType::kImage &&
      response_source != ResponseSource::kFromMemoryCache) {
    KURL resource_url = resource ? resource->Url() : request.Url();
    resource_metrics_by_identifier_.Set(identifier,
                                        ResourceMetrics(resource_url));
  }

  LocalFrameClient* frame_client = frame->Client();

  DCHECK(frame_client);
  if (response_source == ResponseSource::kFromMemoryCache) {
    ResourceRequest resource_request(resource->GetResourceRequest());

    if (!resource_request.Url().ProtocolIs(url::kDataScheme)) {
      frame_client->DispatchDidLoadResourceFromMemoryCache(resource_request,
                                                           response);
      auto scrub_null = [](const String& s) { return s ? s : g_empty_string; };
      frame->GetLocalFrameHostRemote().DidLoadResourceFromMemoryCache(
          resource_request.Url(), scrub_null(resource_request.HttpMethod()),
          scrub_null(response.MimeType()),
          resource_request.GetRequestDestination(),
          response.RequestIncludeCredentials());
    }

    // Note: probe::WillSendRequest needs to precede before this probe method.
    probe::MarkResourceAsCached(frame, document_loader_, identifier);
    if (response.IsNull())
      return;
  }

  RecordAddressSpaceFeature(frame, response);

  document_->Loader()->MaybeRecordServiceWorkerFallbackMainResource(
      response.WasFetchedViaServiceWorker());

  // This used to detect a signed-exchange prefetch here and build an
  // AlternateSignedExchangeResourceInfo from the outer response's "alternate"
  // link headers, to feed LoadLinksFromHeader() below. Signed exchange
  // support is gone (see
  // DocumentLoader::GetPrefetchedSignedExchangeManager()'s old declaration
  // in document_loader.h).

  // Count usage of Content-Disposition header in SVGUse resources.
  if (resource->Options().initiator_info.name ==
          fetch_initiator_type_names::kUse &&
      request.Url().ProtocolIsInHttpFamily() && response.IsAttachment()) {
    CountUsage(WebFeature::kContentDispositionInSvgUse);
  }

  PreloadHelper::LoadLinksFromHeader(
      response.HttpHeaderField(http_names::kLink), response.CurrentRequestUrl(),
      *frame, document_,
      response_source == ResponseSource::kFromMemoryCache
          ? PreloadHelper::LoadLinksFromHeaderMode::kSubresourceFromMemoryCache
          : PreloadHelper::LoadLinksFromHeaderMode::
                kSubresourceNotFromMemoryCache,
      nullptr /* viewport_description */,
      base::OptionalToPtr(response.RecursivePrefetchToken()));

  if (response.HasMajorCertificateErrors()) {
    MixedContentChecker::HandleCertificateError(
        response, request.GetRequestContext(),
        MixedContentChecker::DecideCheckModeForPlugin(frame->GetSettings()),
        document_loader_->GetContentSecurityNotifier());
  }

  frame->Loader().Progress().IncrementProgress(identifier, response);
  probe::DidReceiveResourceResponse(GetProbe(), identifier, document_loader_,
                                    response, resource);
  // It is essential that inspector gets resource response BEFORE console.
  frame->Console().ReportResourceResponseReceived(document_loader_, identifier,
                                                  response);
}

void ResourceLoadObserverForFrame::CheckGuardrailsPolicyForSizeLimit(
    uint64_t identifier,
    uint64_t bytes) {
  auto metrics_it = resource_metrics_by_identifier_.find(identifier);
  if (metrics_it != resource_metrics_by_identifier_.end()) {
    ResourceMetrics& metrics = metrics_it->value;
    metrics.accumulated_bytes += bytes;

    if (document_->GetExecutionContext()->CheckGuardrailsPolicyForAssetSize(
            GuardrailPolicyAssetType::kImage,
            base::saturated_cast<size_t>(metrics.accumulated_bytes),
            metrics.url)) {
      resource_metrics_by_identifier_.erase(identifier);
    }
  }
}

void ResourceLoadObserverForFrame::DidReceiveData(
    uint64_t identifier,
    base::SpanOrSize<const char> chunk) {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  frame->Loader().Progress().IncrementProgress(identifier, chunk.size());
  CheckGuardrailsPolicyForSizeLimit(identifier, chunk.size());
  probe::DidReceiveData(GetProbe(), identifier, document_loader_, chunk);
}

void ResourceLoadObserverForFrame::DidReceiveTransferSizeUpdate(
    uint64_t identifier,
    int transfer_size_diff) {
  DCHECK_GT(transfer_size_diff, 0);
  probe::DidReceiveEncodedDataLength(GetProbe(), document_loader_, identifier,
                                     transfer_size_diff);
}

void ResourceLoadObserverForFrame::DidDownloadToBlob(uint64_t identifier,
                                                     BlobDataHandle* blob) {
  if (blob) {
    probe::DidReceiveBlob(GetProbe(), identifier, document_loader_, blob);
  }
}

void ResourceLoadObserverForFrame::DidFinishLoading(
    uint64_t identifier,
    base::TimeTicks finish_time,
    int64_t encoded_data_length,
    int64_t decoded_body_length) {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  frame->Loader().Progress().CompleteProgress(identifier);
  resource_metrics_by_identifier_.erase(identifier);
  probe::DidFinishLoading(GetProbe(), identifier, document_loader_, finish_time,
                          encoded_data_length, decoded_body_length);

  if (auto* interactive_detector = InteractiveDetector::From(*document_)) {
    interactive_detector->OnResourceLoadEnd(finish_time);
  }
  if (IdlenessDetector* idleness_detector = frame->GetIdlenessDetector()) {
    idleness_detector->OnDidLoadResource();
  }
  document_->CheckCompleted();
}

void ResourceLoadObserverForFrame::DidFailLoading(
    const KURL&,
    uint64_t identifier,
    const ResourceError& error,
    int64_t,
    IsInternalRequest is_internal_request) {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  frame->Loader().Progress().CompleteProgress(identifier);
  resource_metrics_by_identifier_.erase(identifier);

  probe::DidFailLoading(GetProbe(), identifier, document_loader_, error,
                        frame->GetDevToolsFrameToken());

  // Notification to FrameConsole should come AFTER InspectorInstrumentation
  // call, DevTools front-end relies on this.
  if (!is_internal_request) {
    frame->Console().DidFailLoading(document_loader_, identifier, error);
  }
  if (auto* interactive_detector = InteractiveDetector::From(*document_)) {
    // We have not yet recorded load_finish_time. Pass nullopt here; we will
    // call base::TimeTicks::Now() lazily when we need it.
    interactive_detector->OnResourceLoadEnd(std::nullopt);
  }
  if (IdlenessDetector* idleness_detector = frame->GetIdlenessDetector()) {
    idleness_detector->OnDidLoadResource();
  }
  document_->CheckCompleted();
}

void ResourceLoadObserverForFrame::DidChangeRenderBlockingBehavior(
    Resource* resource,
    const FetchParameters& params) {
  // TRACE_EVENT_INSTANT("devtools.timeline", "PreloadRenderBlockingStatusChange",
  //                     base::TimeTicks::Now()) was here. The
  // "devtools.timeline" perfetto category was registered for DevTools'
  // timeline instrumentation, which is gone.
}

bool ResourceLoadObserverForFrame::InterestedInAllRequests() {
  // GetProbe()->HasInspectorNetworkAgents() used to gate this on whether a
  // DevTools network agent was attached and wanted every request observed,
  // not just the render-affecting ones ResourceLoadObserver already reports.
  // There is no DevTools network agent left to attach.
  return false;
}

void ResourceLoadObserverForFrame::Trace(Visitor* visitor) const {
  visitor->Trace(document_loader_);
  visitor->Trace(document_);
  visitor->Trace(fetcher_properties_);
  ResourceLoadObserver::Trace(visitor);
}

CoreProbeSink* ResourceLoadObserverForFrame::GetProbe() {
  return probe::ToCoreProbeSink(*document_);
}

void ResourceLoadObserverForFrame::CountUsage(WebFeature feature) {
  document_loader_->GetUseCounter().Count(feature, document_->GetFrame());
}

}  // namespace blink
