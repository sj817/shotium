// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/public/cpp/resource_request.h"

#include "base/debug/crash_logging.h"
#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/trace_event/typed_macros.h"
#include "base/types/optional_util.h"
#include "net/base/load_flags.h"
#include "net/log/net_log_source.h"
#include "services/network/public/cpp/permissions_policy/permissions_policy.h"
#include "services/network/public/mojom/url_request.mojom.h"

namespace network {

SharedDataPipeProducerHandle::SharedDataPipeProducerHandle(
    mojo::ScopedDataPipeProducerHandle pipe)
    : pipe(std::move(pipe)) {}

SharedDataPipeProducerHandle::~SharedDataPipeProducerHandle() = default;

ResourceRequest::TrustedParams::EnabledClientHints::EnabledClientHints() =
    default;
ResourceRequest::TrustedParams::EnabledClientHints::~EnabledClientHints() =
    default;
ResourceRequest::TrustedParams::EnabledClientHints::EnabledClientHints(
    const EnabledClientHints&) = default;
ResourceRequest::TrustedParams::EnabledClientHints&
ResourceRequest::TrustedParams::EnabledClientHints::operator=(
    const EnabledClientHints&) = default;

bool ResourceRequest::TrustedParams::EnabledClientHints::operator==(
    const EnabledClientHints& other) const {
  return origin == other.origin &&
         is_outermost_main_frame == other.is_outermost_main_frame &&
         hints == other.hints;
}

namespace {

// Returns true iff either holds true:
//
//  - both |lhs| and |rhs| are nullopt, or
//  - neither is nullopt and they both contain equal values
//
bool OptionalTrustedParamsEqualsForTesting(
    const std::optional<ResourceRequest::TrustedParams>& lhs,
    const std::optional<ResourceRequest::TrustedParams>& rhs) {
  return (!lhs && !rhs) || (lhs && rhs && lhs->EqualsForTesting(*rhs));
}

bool OptionalNetLogInfoEqualsForTesting(
    const std::optional<net::NetLogSource>& lhs,
    const std::optional<net::NetLogSource>& rhs) {
  bool equal_members = lhs && rhs && lhs.value() == rhs.value();
  return (!lhs && !rhs) || equal_members;
}

base::debug::CrashKeyString* GetRequestUrlCrashKey() {
  static auto* crash_key = base::debug::AllocateCrashKeyString(
      "request_url", base::debug::CrashKeySize::Size256);
  return crash_key;
}

base::debug::CrashKeyString* GetRequestInitiatorCrashKey() {
  static auto* crash_key = base::debug::AllocateCrashKeyString(
      "request_initiator", base::debug::CrashKeySize::Size64);
  return crash_key;
}

base::debug::CrashKeyString* GetRequestResourceTypeCrashKey() {
  static auto* crash_key = base::debug::AllocateCrashKeyString(
      "request_resource_type", base::debug::CrashKeySize::Size32);
  return crash_key;
}

}  // namespace

ResourceRequest::TrustedParams::TrustedParams() = default;
ResourceRequest::TrustedParams::~TrustedParams() = default;

ResourceRequest::TrustedParams::TrustedParams(const TrustedParams& other) {
  *this = other;
}

ResourceRequest::TrustedParams& ResourceRequest::TrustedParams::operator=(
    const TrustedParams& other) {
  TRACE_EVENT("loading", "ResourceRequest::TrustedParams.copy");
  isolation_info = other.isolation_info;
  disable_secure_dns = other.disable_secure_dns;
  has_user_activation = other.has_user_activation;
  allow_cookies_from_browser = other.allow_cookies_from_browser;
  include_request_cookies_with_response =
      other.include_request_cookies_with_response;
  enabled_client_hints = other.enabled_client_hints;
  client_security_state = other.client_security_state.Clone();
  response_body_stream = other.response_body_stream;
  expected_response_headers_for_synthetic_response =
      other.expected_response_headers_for_synthetic_response;
  return *this;
}

ResourceRequest::TrustedParams::TrustedParams(TrustedParams&& other) = default;
ResourceRequest::TrustedParams& ResourceRequest::TrustedParams::operator=(
    TrustedParams&& other) = default;

bool ResourceRequest::TrustedParams::EqualsForTesting(
    const TrustedParams& other) const {
  return isolation_info.IsEqualForTesting(other.isolation_info) &&
         disable_secure_dns == other.disable_secure_dns &&
         has_user_activation == other.has_user_activation &&
         allow_cookies_from_browser == other.allow_cookies_from_browser &&
         include_request_cookies_with_response ==
             other.include_request_cookies_with_response &&
         enabled_client_hints == other.enabled_client_hints &&
         client_security_state == other.client_security_state &&
         // `response_body_stream` holds a `mojo::ScopedDataPipeProducerHandle`
         // which is moved during serialization. Therefore, we only check for
         // its presence (null or not null) for equality, rather than direct
         // comparison of the refptrs themselves.
         (!!response_body_stream == !!other.response_body_stream) &&
         expected_response_headers_for_synthetic_response ==
             other.expected_response_headers_for_synthetic_response;
}

ResourceRequest::ResourceRequest() = default;
ResourceRequest::ResourceRequest(const ResourceRequest& request) {
  TRACE_EVENT(TRACE_DISABLED_BY_DEFAULT("loading"),
              "ResourceRequest::ResourceRequest.copy_constructor");
  *this = request;
}
ResourceRequest& ResourceRequest::operator=(const ResourceRequest& other) =
    default;
ResourceRequest::ResourceRequest(ResourceRequest&& other) = default;
ResourceRequest& ResourceRequest::operator=(ResourceRequest&& other) = default;
ResourceRequest::~ResourceRequest() = default;

bool ResourceRequest::EqualsForTesting(const ResourceRequest& request) const {
  return method == request.method && url == request.url &&
         site_for_cookies.IsEquivalent(request.site_for_cookies) &&
         update_first_party_url_on_redirect ==
             request.update_first_party_url_on_redirect &&
         request_initiator == request.request_initiator &&
         isolated_world_origin == request.isolated_world_origin &&
         referrer == request.referrer &&
         referrer_policy == request.referrer_policy &&
         headers.ToString() == request.headers.ToString() &&
         cors_exempt_headers.ToString() ==
             request.cors_exempt_headers.ToString() &&
         load_flags == request.load_flags &&
         resource_type == request.resource_type &&
         priority == request.priority &&
         priority_incremental == request.priority_incremental &&

         cors_preflight_policy == request.cors_preflight_policy &&
         originated_from_service_worker ==
             request.originated_from_service_worker &&
         skip_service_worker == request.skip_service_worker &&
         mode == request.mode &&
         required_ip_address_space == request.required_ip_address_space &&
         credentials_mode == request.credentials_mode &&
         redirect_mode == request.redirect_mode &&
         fetch_integrity == request.fetch_integrity &&
         expected_public_keys == request.expected_public_keys &&
         destination == request.destination &&
         request_body == request.request_body &&
         keepalive == request.keepalive &&
         has_user_gesture == request.has_user_gesture &&
         enable_load_timing == request.enable_load_timing &&
         enable_upload_progress == request.enable_upload_progress &&
         do_not_prompt_for_login == request.do_not_prompt_for_login &&
         is_outermost_main_frame == request.is_outermost_main_frame &&
         transition_type == request.transition_type &&
         is_reload_navigation == request.is_reload_navigation &&
         previews_state == request.previews_state &&
         upgrade_if_insecure == request.upgrade_if_insecure &&
         is_revalidating == request.is_revalidating &&
         revalidation_etag == request.revalidation_etag &&
         revalidation_last_modified == request.revalidation_last_modified &&
         throttling_profile_id == request.throttling_profile_id &&
         fetch_window_id == request.fetch_window_id &&
         devtools_request_id == request.devtools_request_id &&
         is_fetch_like_api == request.is_fetch_like_api &&
         is_fetch_later_api == request.is_fetch_later_api &&
         is_favicon == request.is_favicon &&
         recursive_prefetch_token == request.recursive_prefetch_token &&
         OptionalTrustedParamsEqualsForTesting(trusted_params,
                                               request.trusted_params) &&
         devtools_accepted_stream_types ==
             request.devtools_accepted_stream_types &&
         trust_token_params == request.trust_token_params &&
         OptionalNetLogInfoEqualsForTesting(net_log_create_info,
                                            request.net_log_create_info) &&
         OptionalNetLogInfoEqualsForTesting(net_log_reference_info,
                                            request.net_log_reference_info) &&
         shared_dictionary_writer_enabled ==
             request.shared_dictionary_writer_enabled &&
         socket_tag == request.socket_tag &&
         permissions_policy == request.permissions_policy &&
         fetch_retry_options == request.fetch_retry_options;
}

bool ResourceRequest::SendsCookies() const {
  return credentials_mode == network::mojom::CredentialsMode::kInclude;
}

bool ResourceRequest::SavesCookies() const {
  return credentials_mode == network::mojom::CredentialsMode::kInclude &&
         !(load_flags & net::LOAD_DO_NOT_SAVE_COOKIES);
}

void ResourceRequest::UpdateOnRedirect(const net::RedirectInfo& redirect_info) {
  url = redirect_info.new_url;
  method = redirect_info.new_method;
  referrer = GURL(redirect_info.new_referrer);
  referrer_policy = redirect_info.new_referrer_policy;
  site_for_cookies = redirect_info.new_site_for_cookies;

  if (trusted_params) {
    trusted_params->isolation_info =
        trusted_params->isolation_info.CreateForRedirect(
            url::Origin::Create(url));
  }
}

net::ReferrerPolicy ReferrerPolicyForUrlRequest(
    mojom::ReferrerPolicy referrer_policy) {
  switch (referrer_policy) {
    case mojom::ReferrerPolicy::kAlways:
      return net::ReferrerPolicy::NEVER_CLEAR;
    case mojom::ReferrerPolicy::kNever:
      return net::ReferrerPolicy::NO_REFERRER;
    case mojom::ReferrerPolicy::kOrigin:
      return net::ReferrerPolicy::ORIGIN;
    case mojom::ReferrerPolicy::kNoReferrerWhenDowngrade:
      return net::ReferrerPolicy::CLEAR_ON_TRANSITION_FROM_SECURE_TO_INSECURE;
    case mojom::ReferrerPolicy::kOriginWhenCrossOrigin:
      return net::ReferrerPolicy::ORIGIN_ONLY_ON_TRANSITION_CROSS_ORIGIN;
    case mojom::ReferrerPolicy::kSameOrigin:
      return net::ReferrerPolicy::CLEAR_ON_TRANSITION_CROSS_ORIGIN;
    case mojom::ReferrerPolicy::kStrictOrigin:
      return net::ReferrerPolicy::
          ORIGIN_CLEAR_ON_TRANSITION_FROM_SECURE_TO_INSECURE;
    case mojom::ReferrerPolicy::kDefault:
      NOTREACHED();
    case mojom::ReferrerPolicy::kStrictOriginWhenCrossOrigin:
      return net::ReferrerPolicy::REDUCE_GRANULARITY_ON_TRANSITION_CROSS_ORIGIN;
  }
  NOTREACHED();
}

int GetAllowedLoadFlagsForUntrustedRequests() {
  return net::LOAD_VALIDATE_CACHE | net::LOAD_BYPASS_CACHE |
         net::LOAD_SKIP_CACHE_VALIDATION | net::LOAD_ONLY_FROM_CACHE |
         net::LOAD_DISABLE_CACHE | net::LOAD_PREFETCH |
         net::LOAD_IGNORE_LIMITS | net::LOAD_DO_NOT_USE_EMBEDDED_IDENTITY |
         net::LOAD_SUPPORT_ASYNC_REVALIDATION |
         net::LOAD_RESTRICTED_PREFETCH_FOR_MAIN_FRAME;
}

namespace debug {

ScopedResourceRequestCrashKeys::ScopedResourceRequestCrashKeys(
    const network::ResourceRequest& request)
    : url_(GetRequestUrlCrashKey(), request.url.possibly_invalid_spec()),
      request_initiator_(GetRequestInitiatorCrashKey(),
                         base::OptionalToPtr(request.request_initiator)),
      resource_type_(GetRequestResourceTypeCrashKey(),
                     base::NumberToString(request.resource_type)) {}

ScopedResourceRequestCrashKeys::~ScopedResourceRequestCrashKeys() = default;

}  // namespace debug
}  // namespace network
