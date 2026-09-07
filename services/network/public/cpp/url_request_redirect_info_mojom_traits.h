// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_URL_REQUEST_REDIRECT_INFO_MOJOM_TRAITS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_URL_REQUEST_REDIRECT_INFO_MOJOM_TRAITS_H_

#include "mojo/public/cpp/base/time_mojom_traits.h"
#include "mojo/public/cpp/bindings/struct_traits.h"
#include "net/url_request/redirect_info.h"
#include "services/network/public/cpp/cookie_manager_shared_mojom_traits.h"
#include "services/network/public/cpp/url_request_param_mojom_traits.h"
#include "services/network/public/mojom/url_loader.mojom-shared.h"
#include "url/mojom/origin_mojom_traits.h"
#include "url/mojom/url_gurl_mojom_traits.h"

namespace mojo {

template <>
struct StructTraits<network::mojom::URLRequestRedirectInfoDataView,
                    net::RedirectInfo> {
  static const std::optional<url::Origin>& original_initiator(
      const net::RedirectInfo& info) {
    return info.original_initiator;
  }
  static int status_code(const net::RedirectInfo& info) {
    return info.status_code;
  }
  static const std::string& new_method(const net::RedirectInfo& info) {
    return info.new_method;
  }
  static const GURL& new_url(const net::RedirectInfo& info) {
    return info.new_url;
  }
  static const net::SiteForCookies& new_site_for_cookies(
      const net::RedirectInfo& info) {
    return info.new_site_for_cookies;
  }
  static const std::string& new_referrer(const net::RedirectInfo& info) {
    return info.new_referrer;
  }
  static bool insecure_scheme_was_upgraded(const net::RedirectInfo& info) {
    return info.insecure_scheme_was_upgraded;
  }
  static bool is_signed_exchange_fallback_redirect(
      const net::RedirectInfo& info) {
    return info.is_signed_exchange_fallback_redirect;
  }
  static net::ReferrerPolicy new_referrer_policy(const net::RedirectInfo& info) {
    return info.new_referrer_policy;
  }
  static base::TimeTicks critical_ch_restart_time(const net::RedirectInfo& info) {
    return info.critical_ch_restart_time;
  }

  static bool Read(network::mojom::URLRequestRedirectInfoDataView data,
                   net::RedirectInfo* out) {
    out->status_code = data.status_code();
    out->insecure_scheme_was_upgraded = data.insecure_scheme_was_upgraded();
    out->is_signed_exchange_fallback_redirect =
        data.is_signed_exchange_fallback_redirect();
    return data.ReadOriginalInitiator(&out->original_initiator) &&
           data.ReadNewMethod(&out->new_method) &&
           data.ReadNewUrl(&out->new_url) &&
           data.ReadNewSiteForCookies(&out->new_site_for_cookies) &&
           data.ReadNewReferrer(&out->new_referrer) &&
           data.ReadNewReferrerPolicy(&out->new_referrer_policy) &&
           data.ReadCriticalChRestartTime(&out->critical_ch_restart_time);
  }
};

}  // namespace mojo

#endif  // SERVICES_NETWORK_PUBLIC_CPP_URL_REQUEST_REDIRECT_INFO_MOJOM_TRAITS_H_
