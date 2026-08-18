// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/bindings/core/v8/v8_idle_request_options.h"
#include "third_party/blink/renderer/core/loader/preload_helper.h"

#include <utility>

#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "base/rand_util.h"
#include "base/timer/elapsed_timer.h"
#include "services/metrics/public/cpp/ukm_builders.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/mojom/fetch/fetch_api_request.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/public/platform/web_prescient_networking.h"
#include "third_party/blink/renderer/core/css/media_list.h"
#include "third_party/blink/renderer/core/css/media_query_evaluator.h"
#include "third_party/blink/renderer/core/css/parser/sizes_attribute_parser.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/execution_context/agent.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/navigator.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/viewport_data.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/blocking_attribute.h"
#include "third_party/blink/renderer/core/html/parser/html_preload_scanner.h"
#include "third_party/blink/renderer/core/html/parser/html_srcset_parser.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/fetch_priority_attribute.h"
#include "third_party/blink/renderer/core/loader/link_load_parameters.h"
#include "third_party/blink/renderer/core/loader/pending_link_preload.h"
#include "third_party/blink/renderer/core/loader/render_blocking_resource_manager.h"
#include "third_party/blink/renderer/core/loader/resource/css_style_sheet_resource.h"
#include "third_party/blink/renderer/core/loader/resource/font_resource.h"
#include "third_party/blink/renderer/core/loader/resource/image_resource.h"
#include "third_party/blink/renderer/core/loader/resource/link_dictionary_resource.h"
#include "third_party/blink/renderer/core/loader/resource/link_prefetch_resource.h"
#include "third_party/blink/renderer/core/loader/shared_dictionary_hint_type.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/page/viewport_description.h"
#include "third_party/blink/renderer/core/scheduler/scripted_idle_task_controller.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_initiator_type_names.h"
#include "third_party/blink/renderer/platform/loader/fetch/raw_resource.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_fetcher.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_loader_options.h"
#include "third_party/blink/renderer/platform/loader/integrity_report.h"
#include "third_party/blink/renderer/platform/loader/link_header.h"
#include "third_party/blink/renderer/platform/loader/subresource_integrity.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"

namespace blink {

namespace {

class LoadDictionaryWhenIdleTask final : public IdleTask {
 public:
  LoadDictionaryWhenIdleTask(FetchParameters fetch_params,
                             ResourceFetcher* fetcher,
                             PendingLinkPreload* pending_preload)
      : fetch_params_(std::move(fetch_params)),
        resource_fetcher_(fetcher),
        pending_preload_(pending_preload) {}

  void Trace(Visitor* visitor) const override {
    visitor->Trace(resource_fetcher_);
    visitor->Trace(pending_preload_);
    visitor->Trace(fetch_params_);
    IdleTask::Trace(visitor);
  }

 private:
  void invoke(IdleDeadline* deadline) override {
    Resource* resource =
        LinkDictionaryResource::Fetch(fetch_params_, resource_fetcher_);
    if (pending_preload_) {
      pending_preload_->AddResource(resource);
    }
  }

  FetchParameters fetch_params_;
  Member<ResourceFetcher> resource_fetcher_;
  Member<PendingLinkPreload> pending_preload_;
};

void SendMessageToConsoleForPossiblyNullDocument(
    ConsoleMessage* console_message,
    Document* document,
    LocalFrame* frame) {
  DCHECK(document || frame);
  DCHECK(!document || document->GetFrame() == frame);
  // Route the console message through Document if possible, so that script line
  // numbers can be included. Otherwise, route directly to the FrameConsole, to
  // ensure we never drop a message.
  if (document)
    document->AddConsoleMessage(console_message);
  else
    frame->Console().AddMessage(console_message);
}

bool IsSupportedType(ResourceType resource_type, const String& mime_type) {
  if (mime_type.empty())
    return true;
  switch (resource_type) {
    case ResourceType::kImage:
      return MIMETypeRegistry::IsSupportedImagePrefixedMIMEType(mime_type);
    case ResourceType::kScript:
      return MIMETypeRegistry::IsSupportedJavaScriptMIMEType(mime_type);
    case ResourceType::kCSSStyleSheet:
      return MIMETypeRegistry::IsSupportedStyleSheetMIMEType(mime_type);
    case ResourceType::kFont:
      return MIMETypeRegistry::IsSupportedFontMIMEType(mime_type);
    case ResourceType::kAudio:
    case ResourceType::kVideo:
      return MIMETypeRegistry::IsSupportedMediaMIMEType(mime_type, String());
    case ResourceType::kTextTrack:
      return MIMETypeRegistry::IsSupportedTextTrackMIMEType(mime_type);
    case ResourceType::kRaw:
      return true;
    default:
      NOTREACHED();
  }
}

MediaValuesCached* CreateMediaValues(
    Document& document,
    const ViewportDescription* viewport_description) {
  MediaValuesCached* media_values =
      MakeGarbageCollected<MediaValuesCached>(document);
  if (viewport_description) {
    gfx::SizeF initial_viewport(media_values->DeviceWidth(),
                                media_values->DeviceHeight());
    PageScaleConstraints constraints = viewport_description->Resolve(
        initial_viewport, document.GetViewportData().ViewportDefaultMinWidth());
    media_values->OverrideViewportDimensions(constraints.layout_size.width(),
                                             constraints.layout_size.height());
  }
  return media_values;
}

bool MediaMatches(const String& media,
                  MediaValues* media_values,
                  ExecutionContext* execution_context) {
  MediaQuerySet* media_queries =
      MediaQuerySet::Create(media, execution_context);
  MediaQueryEvaluator* evaluator =
      MakeGarbageCollected<MediaQueryEvaluator>(media_values);
  return evaluator->Eval(*media_queries);
}

KURL GetBestFitImageURL(const Document& document,
                        const KURL& base_url,
                        MediaValues* media_values,
                        const KURL& href,
                        const String& image_srcset,
                        const String& image_sizes) {
  float source_size = SizesAttributeParser(media_values, image_sizes,
                                           document.GetExecutionContext())
                          .Size();
  ImageCandidate candidate = BestFitSourceForImageAttributes(
      media_values->DevicePixelRatio(), source_size, href, image_srcset);
  return base_url.IsNull() ? document.CompleteURL(candidate.ToString())
                           : KURL(base_url, candidate.ToString());
}

// Check whether the `as` attribute is valid according to the spec, even if we
// don't currently support it yet.
bool IsValidButUnsupportedAsAttribute(const String& as) {
  DCHECK(as != "fetch" && as != "image" && as != "font" && as != "script" &&
         as != "style" && as != "track");
  return as == "audio" || as == "audioworklet" || as == "document" ||
         as == "embed" || as == "manifest" || as == "object" ||
         as == "paintworklet" || as == "report" || as == "sharedworker" ||
         as == "video" || as == "worker" || as == "xslt";
}

bool IsNetworkHintAllowed(PreloadHelper::LoadLinksFromHeaderMode mode,
                          bool is_header_on_subresource) {
  if (is_header_on_subresource &&
      blink::features::kRestrictLinkHeaderOnSubresourceNetworkHint.Get()) {
    return false;
  }
  switch (mode) {
    case PreloadHelper::LoadLinksFromHeaderMode::kDocumentBeforeCommit:
      return true;
    case PreloadHelper::LoadLinksFromHeaderMode::
        kDocumentAfterCommitWithoutViewport:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::
        kDocumentAfterCommitWithViewport:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::kDocumentAfterLoadCompleted:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::kSubresourceFromMemoryCache:
      return true;
    case PreloadHelper::LoadLinksFromHeaderMode::kSubresourceNotFromMemoryCache:
      return true;
  }
}

bool IsResourceLoadAllowed(PreloadHelper::LoadLinksFromHeaderMode mode,
                           bool is_viewport_dependent,
                           bool is_header_on_subresource) {
  if (is_header_on_subresource &&
      blink::features::kRestrictLinkHeaderOnSubresourceResourceLoad.Get()) {
    return false;
  }
  switch (mode) {
    case PreloadHelper::LoadLinksFromHeaderMode::kDocumentBeforeCommit:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::
        kDocumentAfterCommitWithoutViewport:
      return !is_viewport_dependent;
    case PreloadHelper::LoadLinksFromHeaderMode::
        kDocumentAfterCommitWithViewport:
      return is_viewport_dependent;
    case PreloadHelper::LoadLinksFromHeaderMode::kDocumentAfterLoadCompleted:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::kSubresourceFromMemoryCache:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::kSubresourceNotFromMemoryCache:
      return true;
  }
}

bool IsCompressionDictionaryLoadAllowed(
    PreloadHelper::LoadLinksFromHeaderMode mode,
    bool is_header_on_subresource) {
  if (is_header_on_subresource &&
      blink::features::kRestrictLinkHeaderOnSubresourceCompressionDictionary
          .Get()) {
    return false;
  }
  // Document header can trigger dictionary load after the page load completes.
  // Subresources header can trigger dictionary load if it is not from the
  // memory cache.
  switch (mode) {
    case PreloadHelper::LoadLinksFromHeaderMode::kDocumentBeforeCommit:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::
        kDocumentAfterCommitWithoutViewport:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::
        kDocumentAfterCommitWithViewport:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::kDocumentAfterLoadCompleted:
      return true;
    case PreloadHelper::LoadLinksFromHeaderMode::kSubresourceFromMemoryCache:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::kSubresourceNotFromMemoryCache:
      return true;
  }
}

bool IsSubresourceLoad(PreloadHelper::LoadLinksFromHeaderMode mode) {
  switch (mode) {
    case PreloadHelper::LoadLinksFromHeaderMode::kDocumentBeforeCommit:
    case PreloadHelper::LoadLinksFromHeaderMode::
        kDocumentAfterCommitWithoutViewport:
    case PreloadHelper::LoadLinksFromHeaderMode::
        kDocumentAfterCommitWithViewport:
    case PreloadHelper::LoadLinksFromHeaderMode::kDocumentAfterLoadCompleted:
      return false;
    case PreloadHelper::LoadLinksFromHeaderMode::kSubresourceFromMemoryCache:
    case PreloadHelper::LoadLinksFromHeaderMode::kSubresourceNotFromMemoryCache:
      return true;
    default:
      NOTREACHED();
  }
}

PreloadHelper::OriginStatusOnSubresource GetOriginStatus(bool from_same_origin,
                                                         bool to_same_origin) {
  using OriginStatusOnSubresource = PreloadHelper::OriginStatusOnSubresource;
  if (from_same_origin) {
    if (to_same_origin) {
      return OriginStatusOnSubresource::kFromSameOriginToSameOrigin;
    } else {
      return OriginStatusOnSubresource::kFromSameOriginToCrossOrigin;
    }
  } else {
    if (to_same_origin) {
      return OriginStatusOnSubresource::kFromCrossOriginToSameOrigin;
    } else {
      return OriginStatusOnSubresource::kFromCrossOriginToCrossOrigin;
    }
  }
}

constexpr double kUkmSamplingRate = 0.0025;

}  // namespace

void PreloadHelper::DnsPrefetchIfNeeded(
    const LinkLoadParameters& params,
    Document* document,
    LocalFrame* frame,
    LinkCaller caller) {
  if (document && document->Loader() && document->Loader()->Archive()) {
    return;
  }
  if (params.rel.IsDNSPrefetch()) {
    UseCounter::Count(document, WebFeature::kLinkRelDnsPrefetch);
    if (caller == kLinkCalledFromHeader)
      UseCounter::Count(document, WebFeature::kLinkHeaderDnsPrefetch);
    Settings* settings = frame ? frame->GetSettings() : nullptr;
    // FIXME: The href attribute of the link element can be in "//hostname"
    // form, and we shouldn't attempt to complete that as URL
    // <https://bugs.webkit.org/show_bug.cgi?id=48857>.
    if (settings && settings->GetDNSPrefetchingEnabled() &&
        params.href.IsValid() && !params.href.IsEmpty()) {
      if (settings->GetLogDnsPrefetchAndPreconnect()) {
        SendMessageToConsoleForPossiblyNullDocument(
            MakeGarbageCollected<ConsoleMessage>(
                mojom::blink::ConsoleMessageSource::kOther,
                mojom::blink::ConsoleMessageLevel::kVerbose,
                StrCat({"DNS prefetch triggered for ", params.href.Host()})),
            document, frame);
      }
      WebPrescientNetworking* web_prescient_networking =
          frame ? frame->PrescientNetworking() : nullptr;
      if (web_prescient_networking) {
        web_prescient_networking->PrefetchDNS(params.href);
      }
    }
  }
}

void PreloadHelper::PreconnectIfNeeded(
    const LinkLoadParameters& params,
    Document* document,
    LocalFrame* frame,
    LinkCaller caller) {
  if (document && document->Loader() && document->Loader()->Archive()) {
    return;
  }
  if (params.rel.IsPreconnect() && params.href.IsValid() &&
      params.href.ProtocolIsInHttpFamily()) {
    UseCounter::Count(document, WebFeature::kLinkRelPreconnect);
    if (caller == kLinkCalledFromHeader)
      UseCounter::Count(document, WebFeature::kLinkHeaderPreconnect);
    Settings* settings = frame ? frame->GetSettings() : nullptr;
    if (settings && settings->GetLogDnsPrefetchAndPreconnect()) {
      SendMessageToConsoleForPossiblyNullDocument(
          MakeGarbageCollected<ConsoleMessage>(
              mojom::blink::ConsoleMessageSource::kOther,
              mojom::blink::ConsoleMessageLevel::kVerbose,
              StrCat({"Preconnect triggered for ", params.href.GetString()})),
          document, frame);
      if (params.cross_origin != kCrossOriginAttributeNotSet) {
        SendMessageToConsoleForPossiblyNullDocument(
            MakeGarbageCollected<ConsoleMessage>(
                mojom::blink::ConsoleMessageSource::kOther,
                mojom::blink::ConsoleMessageLevel::kVerbose,
                StrCat({"Preconnect CORS setting is ",
                        (params.cross_origin == kCrossOriginAttributeAnonymous)
                            ? "anonymous"
                            : "use-credentials"})),
            document, frame);
      }
    }
    WebPrescientNetworking* web_prescient_networking =
        frame ? frame->PrescientNetworking() : nullptr;
    if (web_prescient_networking) {
      web_prescient_networking->Preconnect(
          params.href, params.cross_origin != kCrossOriginAttributeAnonymous);
    }
    if (document && document->Fetcher()) {
      document->Fetcher()->RecordPreconnect(params.href, params.cross_origin,
                                            /*early_hints=*/false);
    }
  }
}

// Until the preload cache is defined in terms of range requests and media
// fetches we can't reliably preload audio/video content and expect it to be
// served from the cache correctly. Until
// https://github.com/w3c/preload/issues/97 is resolved and implemented we need
// to disable these preloads.
std::optional<ResourceType> PreloadHelper::GetResourceTypeFromAsAttribute(
    const String& as) {
  DCHECK(as.ContainsNoAsciiUpper());
  if (as == "image")
    return ResourceType::kImage;
  if (as == "script")
    return ResourceType::kScript;
  if (as == "style")
    return ResourceType::kCSSStyleSheet;
  if (as == "track")
    return ResourceType::kTextTrack;
  if (as == "font")
    return ResourceType::kFont;
  if (as == "fetch")
    return ResourceType::kRaw;
  return std::nullopt;
}

// |base_url| is used in Link HTTP Header based preloads to resolve relative
// URLs in srcset, which should be based on the resource's URL, not the
// document's base URL. If |base_url| is a null URL, relative URLs are resolved
// using |document.CompleteURL()|.
void PreloadHelper::PreloadIfNeeded(
    const LinkLoadParameters& params,
    Document& document,
    const KURL& base_url,
    LinkCaller caller,
    const ViewportDescription* viewport_description,
    ParserDisposition parser_disposition,
    PendingLinkPreload* pending_preload) {
  if (!document.Loader() || !params.rel.IsLinkPreload())
    return;

  std::optional<ResourceType> resource_type =
      PreloadHelper::GetResourceTypeFromAsAttribute(params.as);

  MediaValuesCached* media_values = nullptr;
  KURL url;
  if (resource_type == ResourceType::kImage && !params.image_srcset.empty()) {
    UseCounter::Count(document, WebFeature::kLinkRelPreloadImageSrcset);
    media_values = CreateMediaValues(document, viewport_description);
    url = GetBestFitImageURL(document, base_url, media_values, params.href,
                             params.image_srcset, params.image_sizes);
  } else {
    url = params.href;
  }

  UseCounter::Count(document, WebFeature::kLinkRelPreload);
  if (!url.IsValid() || url.IsEmpty()) {
    document.AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::blink::ConsoleMessageSource::kOther,
        mojom::blink::ConsoleMessageLevel::kWarning,
        String("<link rel=preload> has an invalid `href` value")));
    return;
  }

  bool media_matches = true;

  if (!params.media.empty()) {
    if (!media_values)
      media_values = CreateMediaValues(document, viewport_description);
    media_matches = MediaMatches(params.media, media_values,
                                 document.GetExecutionContext());
  }

  DCHECK(pending_preload);

  if (params.reason == LinkLoadParameters::Reason::kMediaChange) {
    if (!media_matches) {
      // Media attribute does not match environment, abort existing preload.
      pending_preload->Dispose();
    } else if (pending_preload->MatchesMedia()) {
      // Media still matches, no need to re-fetch.
      return;
    }
  }

  pending_preload->SetMatchesMedia(media_matches);

  // Preload only if media matches
  if (!media_matches)
    return;

  if (caller == kLinkCalledFromHeader)
    UseCounter::Count(document, WebFeature::kLinkHeaderPreload);
  if (resource_type == std::nullopt) {
    String message;
    if (IsValidButUnsupportedAsAttribute(params.as)) {
      message = String("<link rel=preload> uses an unsupported `as` value");
    } else {
      message = String("<link rel=preload> must have a valid `as` value");
    }
    document.AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::blink::ConsoleMessageSource::kOther,
        mojom::blink::ConsoleMessageLevel::kWarning, message));
    return;
  }
  if (!IsSupportedType(resource_type.value(), params.type)) {
    document.AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::blink::ConsoleMessageSource::kOther,
        mojom::blink::ConsoleMessageLevel::kWarning,
        String("<link rel=preload> has an unsupported `type` value")));
    return;
  }
  ResourceRequest resource_request(url);
  resource_request.SetRequestContext(ResourceFetcher::DetermineRequestContext(
      resource_type.value(), ResourceFetcher::kImageNotImageSet));
  resource_request.SetRequestDestination(
      ResourceFetcher::DetermineRequestDestination(resource_type.value()));

  resource_request.SetReferrerPolicy(params.referrer_policy);

  resource_request.SetFetchPriorityHint(
      GetFetchPriorityAttributeValue(params.fetch_priority_hint));

  ResourceLoaderOptions options;

  options.initiator_info.name = fetch_initiator_type_names::kLink;
  options.parser_disposition = parser_disposition;
  FetchParameters link_fetch_params(std::move(resource_request), options);
  link_fetch_params.SetCharset(document.Encoding());

  if (params.cross_origin != kCrossOriginAttributeNotSet) {
    link_fetch_params.SetCrossOriginAccessControl(
        document.GetExecutionContext()->GetSecurityOrigin(),
        params.cross_origin);
  }

  const String& integrity_attr = params.integrity;
  // A corresponding check for the preload-scanner code path is in
  // TokenPreloadScanner::StartTagScanner::CreatePreloadRequest().
  // TODO(crbug.com/981419): Honor the integrity attribute value for all
  // supported preload destinations, not just the destinations that support SRI
  // in the first place.
  if (resource_type == ResourceType::kScript ||
      resource_type == ResourceType::kCSSStyleSheet ||
      resource_type == ResourceType::kFont) {
    if (!integrity_attr.empty()) {
      IntegrityMetadataSet metadata_set;
      SubresourceIntegrity::ParseIntegrityAttribute(
          integrity_attr, metadata_set, document.GetExecutionContext());
      link_fetch_params.SetIntegrityMetadata(metadata_set);
      link_fetch_params.MutableResourceRequest().SetFetchIntegrity(
          integrity_attr, document.GetExecutionContext());
    }
  } else {
    if (!integrity_attr.empty()) {
      document.AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
          mojom::blink::ConsoleMessageSource::kOther,
          mojom::blink::ConsoleMessageLevel::kWarning,
          String("The `integrity` attribute is currently ignored for preload "
                 "destinations that do not support subresource integrity. See "
                 "https://crbug.com/981419 for more information")));
    }
  }

  link_fetch_params.SetContentSecurityPolicyNonce(params.nonce);
  Settings* settings = document.GetSettings();
  if (settings && settings->GetLogPreload()) {
    String message =
        StrCat({"Preload triggered for ", url.Host(), url.GetPath()});
    String fetch_priority_message;
    if (!params.fetch_priority_hint.empty()) {
      mojom::blink::FetchPriorityHint hint =
          GetFetchPriorityAttributeValue(params.fetch_priority_hint);
      switch (hint) {
        case mojom::blink::FetchPriorityHint::kLow:
          fetch_priority_message = " with fetchpriority hint 'low'";
          break;
        case mojom::blink::FetchPriorityHint::kHigh:
          fetch_priority_message = " with fetchpriority hint 'high'";
          break;
        case mojom::blink::FetchPriorityHint::kAuto:
          fetch_priority_message = " with fetchpriority hint 'auto'";
          break;
        default:
          NOTREACHED();
      }
    }
    document.AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::blink::ConsoleMessageSource::kOther,
        mojom::blink::ConsoleMessageLevel::kVerbose,
        StrCat({message, fetch_priority_message})));
  }
  link_fetch_params.SetLinkPreload(true);
  link_fetch_params.SetRenderBlockingBehavior(
      RenderBlockingBehavior::kNonBlocking);
  if (pending_preload) {
    if (RenderBlockingResourceManager* manager =
            document.GetRenderBlockingResourceManager()) {
      if (EqualIgnoringAsciiCase(params.as, "font")) {
        manager->AddPendingFontPreload(*pending_preload);
      }
    }
  }

  Resource* resource = PreloadHelper::StartPreload(resource_type.value(),
                                                   link_fetch_params, document);
  if (pending_preload)
    pending_preload->AddResource(resource);
}

void PreloadHelper::PrefetchIfNeeded(const LinkLoadParameters& params,
                                     Document& document,
                                     PendingLinkPreload* pending_preload) {
  if (document.Loader() && document.Loader()->Archive())
    return;

  if (!params.rel.IsLinkPrefetch() || !params.href.IsValid() ||
      !document.GetFrame())
    return;
  UseCounter::Count(document, WebFeature::kLinkRelPrefetch);

  ResourceRequest resource_request(params.href);

  bool as_document = EqualIgnoringAsciiCase(params.as, "document");

  // If this corresponds to a preload that we promoted to a prefetch, and the
  // preload had `as="document"`, don't proceed because the original preload
  // statement was invalid.
  if (as_document && params.recursive_prefetch_token) {
    document.AddConsoleMessage(MakeGarbageCollected<ConsoleMessage>(
        mojom::blink::ConsoleMessageSource::kOther,
        mojom::blink::ConsoleMessageLevel::kWarning,
        String("Link header with rel=preload and as=document is unsupported")));
    return;
  }

  // Later a security check is done asserting that the initiator of a
  // cross-origin prefetch request is same-origin with the origin that the
  // browser process is aware of. However, since opaque request initiators are
  // always cross-origin with every other origin, we must not request
  // cross-origin prefetches from opaque requestors.
  if (as_document &&
      !document.GetExecutionContext()->GetSecurityOrigin()->IsOpaque()) {
    resource_request.SetPrefetchMaybeForTopLevelNavigation(true);

    bool is_same_origin =
        document.GetExecutionContext()->GetSecurityOrigin()->IsSameOriginWith(
            SecurityOrigin::Create(params.href).get());
    UseCounter::Count(document,
                      is_same_origin
                          ? WebFeature::kLinkRelPrefetchAsDocumentSameOrigin
                          : WebFeature::kLinkRelPrefetchAsDocumentCrossOrigin);
  }

  // This request could have originally been a preload header on a prefetch
  // response, that was promoted to a prefetch request by LoadLinksFromHeader.
  // In that case, it may have a recursive prefetch token used by the browser
  // process to ensure this request is cached correctly. Propagate it.
  resource_request.SetRecursivePrefetchToken(params.recursive_prefetch_token);

  resource_request.SetReferrerPolicy(params.referrer_policy);
  resource_request.SetFetchPriorityHint(
      GetFetchPriorityAttributeValue(params.fetch_priority_hint));

  ResourceLoaderOptions options;
  options.initiator_info.name = fetch_initiator_type_names::kLink;

  FetchParameters link_fetch_params(std::move(resource_request), options);
  if (params.cross_origin != kCrossOriginAttributeNotSet) {
    link_fetch_params.SetCrossOriginAccessControl(
        document.GetExecutionContext()->GetSecurityOrigin(),
        params.cross_origin);
  }
  Resource* resource =
      LinkPrefetchResource::Fetch(link_fetch_params, document.Fetcher());
  if (pending_preload)
    pending_preload->AddResource(resource);
}

void PreloadHelper::LoadLinksFromHeader(
    const String& header_value,
    const KURL& base_url,
    LocalFrame& frame,
    Document* document,
    LoadLinksFromHeaderMode mode,
    const ViewportDescription* viewport_description,
    const base::UnguessableToken* recursive_prefetch_token) {
  if (header_value.empty())
    return;

  base::UmaHistogramEnumeration("Blink.LinkHeader.LoadLinksFromHeaderMode",
                                mode);

  const bool is_subresource_load = IsSubresourceLoad(mode);
  const bool from_same_origin =
      document ? document->GetExecutionContext()
                     ->GetSecurityOrigin()
                     ->IsSameOriginWith(SecurityOrigin::Create(base_url).get())
               : false;

  LinkHeaderSet header_set(header_value);
  for (auto& header : header_set) {
    if (!header.Valid() || header.Url().empty() || header.Rel().empty()) {
      continue;
    }
    bool is_network_hint_allowed =
        IsNetworkHintAllowed(mode, is_subresource_load);
    bool is_resource_load_allowed = IsResourceLoadAllowed(
        mode, header.IsViewportDependent(), is_subresource_load);
    bool is_compression_dictionary_load_allowed =
        IsCompressionDictionaryLoadAllowed(mode, is_subresource_load);
    if (!is_network_hint_allowed && !is_resource_load_allowed &&
        !is_compression_dictionary_load_allowed) {
      // Skip this `header`; it won't initiate any types of preloading.
      continue;
    }

    LinkLoadParameters params(header, base_url);
    bool change_rel_to_prefetch = false;

    // Record UKM by the rate of `kUkmSamplingRate` to avoid UKM infra's
    // automatic downsampling.
    if (is_subresource_load && base::RandDouble() < kUkmSamplingRate) {
      CHECK(document);
      bool to_same_origin =
          document->GetExecutionContext()
              ->GetSecurityOrigin()
              ->IsSameOriginWith(SecurityOrigin::Create(params.href).get());
      const OriginStatusOnSubresource origin_status =
          GetOriginStatus(from_same_origin, to_same_origin);
      ukm::builders::Blink_Preloading_ByLinkHeader(document->UkmSourceID())
          .SetOriginStatusOnSubresource(std::to_underlying(origin_status))
          .Record(document->UkmRecorder());
    }
    if (is_subresource_load && !from_same_origin &&
        blink::features::kRestrictLinkHeaderOnSubresourceCrossOrigin.Get()) {
      continue;
    }

    // For security purposes, set `referrerpolicy: "no-referrer"` in link loads
    // from subresources. See https://crbug.com/415810136 for details.
    if (base::FeatureList::IsEnabled(
            blink::features::kNoReferrerForPreloadFromSubresource)) {
      if (is_subresource_load) {
        params.referrer_policy = network::mojom::ReferrerPolicy::kNever;
      }
    }

    if (params.rel.IsLinkPreload() && recursive_prefetch_token) {
      // Only preload headers are expected to have a recursive prefetch token
      // In response to that token's existence, we treat the request as a
      // prefetch.
      params.recursive_prefetch_token = *recursive_prefetch_token;
      change_rel_to_prefetch = true;
    }

    // This used to rewrite `params.href` to an alternate signed-exchange
    // subresource URL here when `alternate_resource_info` had a matching
    // entry. Signed exchange support is gone (see
    // DocumentLoader::GetPrefetchedSignedExchangeManager()'s old declaration
    // in document_loader.h), so there is no longer a source for that info.

    if (change_rel_to_prefetch) {
      params.rel = LinkRelAttribute("prefetch");
    }

    // Sanity check to avoid re-entrancy here.
    if (params.href == base_url) {
      continue;
    }
    if (is_network_hint_allowed) {
      DnsPrefetchIfNeeded(params, document, &frame, kLinkCalledFromHeader);

      PreconnectIfNeeded(params, document, &frame, kLinkCalledFromHeader);
    }
    if (is_resource_load_allowed || is_compression_dictionary_load_allowed) {
      DCHECK(document);
      PendingLinkPreload* pending_preload =
          MakeGarbageCollected<PendingLinkPreload>(*document,
                                                   nullptr /* LinkLoader */);
      document->AddPendingLinkHeaderPreload(*pending_preload);
      if (is_resource_load_allowed) {
        PreloadIfNeeded(params, *document, base_url, kLinkCalledFromHeader,
                        viewport_description, kNotParserInserted,
                        pending_preload);
        PrefetchIfNeeded(params, *document, pending_preload);
      }
      if (is_compression_dictionary_load_allowed) {
        if (params.rel.IsCompressionDictionary()) {
          base::UmaHistogramEnumeration("Blink.SharedDictionary.Hint.Discovery",
                                        SharedDictionaryHintType::kHttpHeader);
        }
        FetchCompressionDictionaryIfNeeded(params, *document, pending_preload);
      }
    }
    if (params.rel.IsServiceWorker()) {
      UseCounter::Count(document, WebFeature::kLinkHeaderServiceWorker);
    }
    // TODO(yoav): Add more supported headers as needed.
  }
}

// TODO(crbug.com/1413922):
// Always load the resource after the full document load completes
void PreloadHelper::FetchCompressionDictionaryIfNeeded(
    const LinkLoadParameters& params,
    Document& document,
    PendingLinkPreload* pending_preload) {
  if (!document.Loader() || document.Loader()->Archive()) {
    return;
  }

  if (!params.rel.IsCompressionDictionary() || !params.href.IsValid() ||
      !document.GetFrame()) {
    return;
  }

  DVLOG(1) << "PreloadHelper::FetchCompressionDictionaryIfNeeded "
           << params.href.GetString().Utf8();
  ResourceRequest resource_request(params.href);

  if (!RuntimeEnabledFeatures::CDTNewCrossOriginHandlingEnabled()) {
    resource_request.SetMode(network::mojom::RequestMode::kCors);
    resource_request.SetCredentialsMode(network::mojom::CredentialsMode::kOmit);
  }
  if (RuntimeEnabledFeatures::
          CDTNewReferrerAndReferrerPolicyHandlingEnabled()) {
    resource_request.SetReferrerPolicy(params.referrer_policy);
  } else {
    resource_request.SetReferrerString(Referrer::NoReferrer());
    resource_request.SetReferrerPolicy(network::mojom::ReferrerPolicy::kNever);
  }
  resource_request.SetRequestDestination(
      network::mojom::RequestDestination::kCompressionDictionary);

  ResourceLoaderOptions options;
  options.initiator_info.name = fetch_initiator_type_names::kLink;

  FetchParameters link_fetch_params(std::move(resource_request), options);
  if (RuntimeEnabledFeatures::CDTNewCrossOriginHandlingEnabled()) {
    CrossOriginAttributeValue cross_origin = params.cross_origin;
    // Default to anonymous.
    // https://github.com/whatwg/html/pull/11620
    if (cross_origin == kCrossOriginAttributeNotSet) {
      cross_origin = kCrossOriginAttributeAnonymous;
    }
    link_fetch_params.SetCrossOriginAccessControl(
        document.GetExecutionContext()->GetSecurityOrigin(), cross_origin);
  }
  IdleRequestOptions* idle_options = IdleRequestOptions::Create();
  ScriptedIdleTaskController::From(*document.GetExecutionContext())
      .RegisterCallback(MakeGarbageCollected<LoadDictionaryWhenIdleTask>(
                            std::move(link_fetch_params), document.Fetcher(),
                            pending_preload),
                        idle_options);
}

Resource* PreloadHelper::StartPreload(ResourceType type,
                                      FetchParameters& params,
                                      Document& document) {
  base::ElapsedTimer timer;

  ResourceFetcher* resource_fetcher = document.Fetcher();
  Resource* resource = nullptr;
  switch (type) {
    case ResourceType::kImage:
      resource = ImageResource::Fetch(params, resource_fetcher);
      break;
    case ResourceType::kScript:
      // This build has no script engine, so a script can never be executed and
      // its bytes would never be read by anything. Skip the fetch entirely
      // rather than spend a request on it. `resource` stays null; every caller
      // of StartPreload() already handles a null return (it is also what the
      // unsupported-type path below produces).
      break;
    case ResourceType::kCSSStyleSheet:
      resource =
          CSSStyleSheetResource::Fetch(params, resource_fetcher, nullptr);
      break;
    case ResourceType::kFont:
      resource = FontResource::Fetch(params, resource_fetcher, nullptr);
      if (document.GetRenderBlockingResourceManager()) {
        document.GetRenderBlockingResourceManager()
            ->EnsureStartFontPreloadMaxBlockingTimer();
      }
      document.CountUse(mojom::blink::WebFeature::kLinkRelPreloadAsFont);
      break;
    case ResourceType::kAudio:
    case ResourceType::kVideo:
      params.MutableResourceRequest().SetUseStreamOnResponse(true);
      params.MutableOptions().data_buffering_policy = kDoNotBufferData;
      resource = RawResource::FetchMedia(params, resource_fetcher, nullptr);
      break;
    case ResourceType::kTextTrack:
      params.MutableResourceRequest().SetUseStreamOnResponse(true);
      params.MutableOptions().data_buffering_policy = kDoNotBufferData;
      resource = RawResource::FetchTextTrack(params, resource_fetcher, nullptr);
      break;
    case ResourceType::kRaw:
      params.MutableResourceRequest().SetUseStreamOnResponse(true);
      params.MutableOptions().data_buffering_policy = kDoNotBufferData;
      resource = RawResource::Fetch(params, resource_fetcher, nullptr);
      break;
    default:
      NOTREACHED();
  }

  base::UmaHistogramMicrosecondsTimes("Blink.PreloadRequestStartDuration",
                                      timer.Elapsed());

  return resource;
}

}  // namespace blink
