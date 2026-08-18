// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/base_fetch_context.h"

#include "base/command_line.h"
#include "services/network/public/cpp/connection_allowlist.h"
#include "services/network/public/cpp/features.h"
#include "services/network/public/cpp/request_mode.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/common/switches.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom-blink.h"
#include "third_party/blink/public/mojom/loader/request_context_frame_type.mojom-blink.h"
#include "third_party/blink/public/mojom/service_worker/controller_service_worker_mode.mojom-blink.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/connection_allowlist_violation_report_body.h"
#include "third_party/blink/renderer/core/frame/integrity_policy.h"
#include "third_party/blink/renderer/core/frame/policy_container.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/loader/frame_client_hints_preferences_context.h"
#include "third_party/blink/renderer/platform/exported/wrapped_resource_request.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/loader/cors/cors.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher_properties.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_load_priority.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loading_log.h"
#include "third_party/blink/renderer/platform/network/network_state_notifier.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/scheme_registry.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"

namespace blink {

std::optional<ResourceRequestBlockedReason> BaseFetchContext::CanRequest(
    ResourceType type,
    const ResourceRequest& resource_request,
    const KURL& url,
    const ResourceLoaderOptions& options,
    ReportingDisposition reporting_disposition,
    base::optional_ref<const ResourceRequest::RedirectInfo> redirect_info)
    const {
  std::optional<ResourceRequestBlockedReason> blocked_reason =
      CanRequestInternal(type, resource_request, url, options,
                         reporting_disposition, redirect_info);
  if (blocked_reason &&
      reporting_disposition == ReportingDisposition::kReport) {
    DispatchDidBlockRequest(resource_request, options, blocked_reason.value(),
                            type);
  }
  return blocked_reason;
}

// CanRequestBasedOnSubresourceFilterOnly() and CalculateResourceAnnotations()
// were here. Both consulted the SubresourceFilter -- the first to block a load,
// the second to annotate it as an ad. //components/subresource_filter and the
// content/renderer agent that built the filter are gone, and nothing else
// implements WebDocumentSubresourceFilter, so there is no ruleset to match a
// URL against.

void BaseFetchContext::PrintAccessDeniedMessage(const KURL& url) const {
  if (url.IsNull()) {
    return;
  }

  String message;
  StringView prefix("Unsafe attempt to load URL ");
  if (Url().IsNull()) {
    message = StrCat({prefix, url.ElidedString(), "."});
  } else {
    message =
        StrCat({prefix, url.ElidedString(), " from frame with URL ",
                Url().ElidedString(),
                url.IsLocalFile() || Url().IsLocalFile()
                    ? ". 'file:' URLs are treated as unique security origins.\n"
                    : ". Domains, protocols and ports must match.\n"});
  }

  console_logger_->AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
      mojom::ConsoleMessageSource::kSecurity,
      mojom::ConsoleMessageLevel::kError, message));
}

std::optional<ResourceRequestBlockedReason>
BaseFetchContext::CheckCSPForRequest(
    mojom::blink::RequestContextType request_context,
    network::mojom::RequestDestination request_destination,
    network::mojom::RequestMode request_mode,
    const KURL& url,
    const ResourceLoaderOptions& options,
    ReportingDisposition reporting_disposition,
    const KURL& url_before_redirects,
    ResourceRequest::RedirectStatus redirect_status) const {
  return CheckCSPForRequestInternal(
      request_context, request_destination, request_mode, url, options,
      reporting_disposition, url_before_redirects, redirect_status,
      ContentSecurityPolicy::CheckHeaderType::kCheckReportOnly);
}

std::optional<ResourceRequestBlockedReason>
BaseFetchContext::CheckAndEnforceCSPForRequest(
    mojom::blink::RequestContextType request_context,
    network::mojom::RequestDestination request_destination,
    network::mojom::RequestMode request_mode,
    const KURL& url,
    const ResourceLoaderOptions& options,
    ReportingDisposition reporting_disposition,
    const KURL& url_before_redirects,
    ResourceRequest::RedirectStatus redirect_status) const {
  return CheckCSPForRequestInternal(
      request_context, request_destination, request_mode, url, options,
      reporting_disposition, url_before_redirects, redirect_status,
      ContentSecurityPolicy::CheckHeaderType::kCheckAll);
}

std::optional<ResourceRequestBlockedReason>
BaseFetchContext::CheckCSPForRequestInternal(
    mojom::blink::RequestContextType request_context,
    network::mojom::RequestDestination request_destination,
    network::mojom::RequestMode request_mode,
    const KURL& url,
    const ResourceLoaderOptions& options,
    ReportingDisposition reporting_disposition,
    const KURL& url_before_redirects,
    ResourceRequest::RedirectStatus redirect_status,
    ContentSecurityPolicy::CheckHeaderType check_header_type) const {
  if (options.content_security_policy_option ==
      network::mojom::CSPDisposition::DO_NOT_CHECK) {
    return std::nullopt;
  }

  // There are no isolated worlds without a script engine, so there is only
  // ever the document's own CSP to check against.
  ContentSecurityPolicy* csp = GetContentSecurityPolicy();

  if (csp &&
      !csp->AllowRequest(request_context, request_destination, request_mode,
                         url, options.content_security_policy_nonce,
                         options.integrity_metadata, options.parser_disposition,
                         url_before_redirects, redirect_status,
                         reporting_disposition, check_header_type)) {
    return ResourceRequestBlockedReason::kCSP;
  }

  return std::nullopt;
}

std::optional<ResourceRequestBlockedReason>
BaseFetchContext::CanRequestInternal(
    ResourceType type,
    const ResourceRequest& resource_request,
    const KURL& url,
    const ResourceLoaderOptions& options,
    ReportingDisposition reporting_disposition,
    base::optional_ref<const ResourceRequest::RedirectInfo> redirect_info)
    const {
  if (GetResourceFetcherProperties().IsDetached()) {
    if (!resource_request.GetKeepalive() || !redirect_info.has_value()) {
      return ResourceRequestBlockedReason::kOther;
    }
  }

  if (ShouldBlockRequestByInspector(resource_request.Url())) {
    return ResourceRequestBlockedReason::kInspector;
  }

  mojom::blink::RequestContextType request_context =
      resource_request.GetRequestContext();
  network::mojom::RequestDestination request_destination =
      resource_request.GetRequestDestination();
  const auto request_mode = resource_request.GetMode();

  scoped_refptr<const SecurityOrigin> origin =
      resource_request.RequestorOrigin();

  // On navigation cases, Context().GetSecurityOrigin() may return nullptr, so
  // the request's origin may be nullptr.
  // TODO(yhirano): Figure out if it's actually fine.
  CHECK(request_mode == network::mojom::RequestMode::kNavigate || origin);
  if (request_mode != network::mojom::RequestMode::kNavigate &&
      !resource_request.CanDisplay(url)) {
    if (reporting_disposition == ReportingDisposition::kReport) {
      console_logger_->AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
          mojom::ConsoleMessageSource::kJavaScript,
          mojom::ConsoleMessageLevel::kError,
          StrCat({"Not allowed to load local resource: ", url.GetString()})));
    }
    RESOURCE_LOADING_DVLOG(1) << "ResourceFetcher::requestResource URL was not "
                                 "allowed by SecurityOrigin::CanDisplay";
    return ResourceRequestBlockedReason::kOther;
  }

  if (!url.ProtocolIsData()) {
    // CORS is defined only for HTTP(S) requests. See
    // https://fetch.spec.whatwg.org/#http-extensions.
    if (request_mode == network::mojom::RequestMode::kSameOrigin &&
        cors::CalculateCorsFlag(url, origin.get(),
                                resource_request.IsolatedWorldOrigin().get(),
                                request_mode)) {
      PrintAccessDeniedMessage(url);
      return ResourceRequestBlockedReason::kOrigin;
    }
  }

  // User Agent CSS stylesheets should only support loading images and should
  // be restricted to data urls.
  if (options.initiator_info.name == fetch_initiator_type_names::kUacss) {
    if (type == ResourceType::kImage && url.ProtocolIsData()) {
      return std::nullopt;
    }
    return ResourceRequestBlockedReason::kOther;
  }

  const KURL& url_before_redirects =
      redirect_info.has_value() ? redirect_info->original_url : url;
  const ResourceRequestHead::RedirectStatus redirect_status =
      redirect_info.has_value()
          ? ResourceRequestHead::RedirectStatus::kFollowedRedirect
          : ResourceRequestHead::RedirectStatus::kNoRedirect;
  // We check the 'report-only' headers before upgrading the request (in
  // populateResourceRequest). We check the enforced headers here to ensure
  // we block things we ought to block.
  if (CheckCSPForRequestInternal(
          request_context, request_destination, request_mode, url, options,
          reporting_disposition, url_before_redirects, redirect_status,
          ContentSecurityPolicy::CheckHeaderType::kCheckEnforce) ==
      ResourceRequestBlockedReason::kCSP) {
    return ResourceRequestBlockedReason::kCSP;
  }

  CHECK(!GetResourceFetcherProperties().IsDetached() ||
        resource_request.GetKeepalive() || redirect_info.has_value());

  if (!IntegrityPolicy::AllowRequest(GetExecutionContext(), request_destination,
                                     request_mode, options.integrity_metadata,
                                     url)) {
    return ResourceRequestBlockedReason::kIntegrity;
  }

  if (type == ResourceType::kScript) {
    if (!AllowScript()) {
      // TODO(estark): Use a different ResourceRequestBlockedReason here, since
      // this check has nothing to do with CSP. https://crbug.com/600795
      return ResourceRequestBlockedReason::kCSP;
    }
  }

  // SVG images/resource documents have unique security rules that prevent all
  // subresource requests except for data urls.
  if (IsIsolatedSVGChromeClient() && !url.ProtocolIsData()) {
    return ResourceRequestBlockedReason::kOrigin;
  }

  // data: URL is deprecated in SVGUseElement.
  if (RuntimeEnabledFeatures::RemoveDataUrlInSvgUseEnabled() &&
      options.initiator_info.name == fetch_initiator_type_names::kUse &&
      url.ProtocolIsData() &&
      !base::CommandLine::ForCurrentProcess()->HasSwitch(
          blink::switches::kDataUrlInSvgUseEnabled)) {
    PrintAccessDeniedMessage(url);
    return ResourceRequestBlockedReason::kOrigin;
  }

  // Nothing below this point applies to data: URL images.
  if (type == ResourceType::kImage && url.ProtocolIsData()) {
    return std::nullopt;
  }

  // Measure the number of embedded-credential ('http://user:password@...')
  // resources embedded as subresources.
  const FetchClientSettingsObject& fetch_client_settings_object =
      GetResourceFetcherProperties().GetFetchClientSettingsObject();
  const SecurityOrigin* embedding_origin =
      fetch_client_settings_object.GetSecurityOrigin();
  DCHECK(embedding_origin);
  if (ShouldBlockFetchAsCredentialedSubresource(resource_request, url)) {
    return ResourceRequestBlockedReason::kOrigin;
  }

  // Check for mixed content. We do this second-to-last so that when folks block
  // mixed content via CSP, they don't get a mixed content warning, but a CSP
  // warning instead.
  if (ShouldBlockFetchByMixedContentCheck(
          request_context, resource_request.GetTargetAddressSpace(),
          redirect_info, url, reporting_disposition,
          resource_request.GetDevToolsId())) {
    return ResourceRequestBlockedReason::kMixedContent;
  }

  if (url.PotentiallyDanglingMarkup() && url.ProtocolIsInHttpFamily()) {
    CountDeprecation(WebFeature::kCanRequestURLHTTPContainingNewline);
    return ResourceRequestBlockedReason::kOther;
  }

  // Enforce Connection-Allowlist when the document is controlled by a service
  // worker. Only perform enforcement when we don't have a redirect_info;
  // If the request has reached the point where it has been redirected, or
  // synthetic redirect info is provided in the case of post-request checks,
  // then Connection-Allowlist checks for the request should have already
  // occurred in the network service URLLoaderFactory checks.
  if (base::FeatureList::IsEnabled(network::features::kConnectionAllowlists) &&
      GetResourceFetcherProperties().GetControllerServiceWorkerMode() !=
          mojom::blink::ControllerServiceWorkerMode::kNoController &&
      !redirect_info.has_value()) {
    if (GetExecutionContext() && GetExecutionContext()->GetPolicyContainer()) {
      const auto& policies =
          GetExecutionContext()->GetPolicyContainer()->GetPolicies();

      auto check_allowlist_and_report =
          [&](const network::ConnectionAllowlist& allowlist,
              const V8ConnectionAllowlistDisposition::Enum ca_disposition) {
            bool matched =
                network::ConnectionAllowlistMatchesUrl(allowlist, GURL(url));
            if (!matched) {
              if (reporting_disposition == ReportingDisposition::kReport) {
                PrintAccessDeniedMessage(url);
                ConnectionAllowlistViolationReportBody::
                    QueueServiceWorkerReport(url, ca_disposition,
                                             *GetExecutionContext());
              }
            }
            return matched;
          };

      if (policies.connection_allowlists.report_only.has_value()) {
        check_allowlist_and_report(
            policies.connection_allowlists.report_only.value(),
            V8ConnectionAllowlistDisposition::Enum::kReport);
      }
      if (policies.connection_allowlists.enforced.has_value() &&
          !check_allowlist_and_report(
              policies.connection_allowlists.enforced.value(),
              V8ConnectionAllowlistDisposition::Enum::kEnforce)) {
        return ResourceRequestBlockedReason::kOther;
      }
    }
  }

  // The subresource filter had the final say on whether the load proceeded.
  // There is no filter, so CanRequestInternal now ends on the checks above.

  return std::nullopt;
}

void BaseFetchContext::Trace(Visitor* visitor) const {
  visitor->Trace(fetcher_properties_);
  visitor->Trace(console_logger_);
  FetchContext::Trace(visitor);
}

}  // namespace blink
