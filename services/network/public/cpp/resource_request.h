// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_REQUEST_H_
#define SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_REQUEST_H_

#include <stdint.h>

#include <optional>
#include <string>

#include "base/component_export.h"
#include "base/memory/scoped_refptr.h"
#include "base/unguessable_token.h"
#include "net/base/isolation_info.h"
#include "net/base/request_priority.h"
#include "net/cookies/site_for_cookies.h"
#include "net/http/http_request_headers.h"
#include "net/socket/socket_tag.h"
#include "net/storage_access_api/status.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/referrer_policy.h"
#include "services/network/public/cpp/optional_trust_token_params.h"
#include "services/network/public/cpp/permissions_policy/permissions_policy.h"
#include "services/network/public/cpp/resource_request_body.h"
#include "services/network/public/mojom/client_security_state.mojom.h"
#include "services/network/public/mojom/cors.mojom-shared.h"
#include "services/network/public/mojom/fetch_api.mojom-shared.h"
#include "services/network/public/mojom/ip_address_space.mojom-shared.h"
#include "services/network/public/mojom/referrer_policy.mojom-shared.h"
#include "services/network/public/mojom/trust_tokens.mojom.h"
#include "services/network/public/mojom/url_request.mojom-forward.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace network {
class ResourceRequestBody;

// Typemapped to network.mojom.URLRequest in url_request.mojom.
//
// Note: Please revise EqualsForTesting accordingly on any updates to this
// struct.
struct COMPONENT_EXPORT(NETWORK_CPP_BASE) ResourceRequest {
  // Typemapped to network.mojom.TrustedUrlRequestParams, see comments there
  // for details of each field.
  //
  // TODO(mmenke):  There are likely other fields that should be moved into this
  // class.
  struct COMPONENT_EXPORT(NETWORK_CPP_BASE) TrustedParams {
    TrustedParams();
    ~TrustedParams();
    // TODO(crbug.com/332706093): Make this move-only to avoid cloning mojo
    // interfaces.
    TrustedParams(const TrustedParams& params);
    TrustedParams& operator=(const TrustedParams& other);
    TrustedParams(TrustedParams&& other);
    TrustedParams& operator=(TrustedParams&& other);

    bool EqualsForTesting(const TrustedParams& other) const;

    net::IsolationInfo isolation_info;
    bool disable_secure_dns = false;
    bool has_user_activation = false;

    mojom::ClientSecurityStatePtr client_security_state;
  };

  ResourceRequest();
  ResourceRequest(const ResourceRequest& request);
  ResourceRequest& operator=(const ResourceRequest& other);
  ResourceRequest(ResourceRequest&& other);
  ResourceRequest& operator=(ResourceRequest&& other);

  ~ResourceRequest();

  bool EqualsForTesting(const ResourceRequest& request) const;
  bool SendsCookies() const;
  bool SavesCookies() const;

  // Updates `*this` upon a redirect. This method is to deduplicate the common
  // `ResourceRequest` modification code based on `RedirectInfo`.
  // TODO(crbug.com/434292502): Unify more call sites. For example, there are
  // code locations that do the same thing except for calling
  // `CreateForRedirect()`. Perhaps such code locations are mergeable, because
  // `CreateForRedirect()` is no-op when for `RequestType::kOther`.
  void UpdateOnRedirect(const net::RedirectInfo& redirect_info);

  // See comments in network.mojom.URLRequest in url_request.mojom for details
  // of each field.
  // LINT.IfChange(ResourceRequestFields)
  std::string method = net::HttpRequestHeaders::kGetMethod;
  GURL url;
  net::SiteForCookies site_for_cookies;
  bool update_first_party_url_on_redirect = false;

  // SECURITY NOTE: |request_initiator| is a security-sensitive field.  Please
  // consult the doc comment for |request_initiator| in url_request.mojom.
  std::optional<url::Origin> request_initiator;

  // TODO(crbug.com/40137011): Remove the `isolated_world_origin` field
  // once Chrome Platform Apps are gone.
  std::optional<url::Origin> isolated_world_origin;

  // The chain of URLs seen during navigation redirects.  This should only
  // contain values if the mode is `RedirectMode::kNavigate`.
  std::vector<GURL> navigation_redirect_chain;

  GURL referrer;
  net::ReferrerPolicy referrer_policy = net::ReferrerPolicy::NEVER_CLEAR;
  net::HttpRequestHeaders headers;
  net::HttpRequestHeaders cors_exempt_headers;
  int load_flags = 0;
  // Note: kMainFrame is used only for outermost main frames, i.e. fenced
  // frames are considered a kSubframe for ResourceType.
  int resource_type = 0;
  net::RequestPriority priority = net::IDLE;
  bool priority_incremental = net::kDefaultPriorityIncremental;
  mojom::CorsPreflightPolicy cors_preflight_policy =
      mojom::CorsPreflightPolicy::kConsiderPreflight;
  bool skip_service_worker = false;
  // `kNoCors` mode is the default request mode for legacy reasons, however this
  // mode is highly discouraged for new requests made on the web platform;
  // please consider using another mode like `kCors` instead, and only use
  // `kNoCors` with strong rationale and approval from security experts. See
  // https://fetch.spec.whatwg.org/#concept-request-mode.
  mojom::RequestMode mode = mojom::RequestMode::kNoCors;
  mojom::IPAddressSpace required_ip_address_space =
      mojom::IPAddressSpace::kUnknown;
  mojom::CredentialsMode credentials_mode = mojom::CredentialsMode::kInclude;
  mojom::RedirectMode redirect_mode = mojom::RedirectMode::kFollow;
  // Exposed as Request.integrity in Service Workers
  std::string fetch_integrity;
  // Used to populate `Accept-Signatures`
  // https://www.rfc-editor.org/rfc/rfc9421.html#name-the-accept-signature-field
  std::vector<std::vector<uint8_t>> expected_public_keys;
  mojom::RequestDestination destination = mojom::RequestDestination::kEmpty;
  mojom::RequestDestination original_destination =
      mojom::RequestDestination::kEmpty;
  scoped_refptr<ResourceRequestBody> request_body;
  bool keepalive = false;
  bool has_user_gesture = false;
  bool enable_load_timing = false;
  bool enable_upload_progress = false;
  bool do_not_prompt_for_login = false;
  bool is_outermost_main_frame = false;

  bool is_reload_navigation = false;

  bool upgrade_if_insecure = false;
  bool is_revalidating = false;
  std::optional<std::string> revalidation_etag;
  std::optional<std::string> revalidation_last_modified;
  bool is_fetch_like_api = false;
  bool is_fetch_later_api = false;
  bool is_favicon = false;
  std::optional<base::UnguessableToken> recursive_prefetch_token;
  std::optional<TrustedParams> trusted_params;
  // |trust_token_params| uses a custom std::optional-like type to make the
  // field trivially copyable; see OptionalTrustTokenParams's definition for
  // more context.
  OptionalTrustTokenParams trust_token_params;
  net::StorageAccessApiStatus storage_access_api_status =
      net::StorageAccessApiStatus::kNone;

  bool is_ad_tagged = false;
  bool client_side_content_decoding_enabled = false;
  net::SocketTag socket_tag;

  std::optional<network::PermissionsPolicy> permissions_policy;

  // LINT.ThenChange(//services/network/prefetch_matches.cc)
};

// This does not accept |kDefault| referrer policy.
COMPONENT_EXPORT(NETWORK_CPP_BASE)
net::ReferrerPolicy ReferrerPolicyForUrlRequest(
    mojom::ReferrerPolicy referrer_policy);

// Returns a bitmask of net::LOAD_* flags that are allowed for requests from
// untrusted processes.
COMPONENT_EXPORT(NETWORK_CPP_BASE)
int GetAllowedLoadFlagsForUntrustedRequests();

}  // namespace network

#endif  // SERVICES_NETWORK_PUBLIC_CPP_RESOURCE_REQUEST_H_
