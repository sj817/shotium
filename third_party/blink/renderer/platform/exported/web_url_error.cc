// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_url_error.h"

#include "net/base/net_errors.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace blink {
namespace {

WebURLError CreateInternal(const network::URLLoaderCompletionStatus& status,
                           const WebURL& url) {
  const WebURLError::HasCopyInCache has_copy_in_cache =
      status.exists_in_cache ? WebURLError::HasCopyInCache::kTrue
                             : WebURLError::HasCopyInCache::kFalse;
  if (status.cors_error_status)
    return WebURLError(*status.cors_error_status, has_copy_in_cache, url);
  if (status.blocked_by_response_reason) {
    DCHECK_EQ(net::ERR_BLOCKED_BY_RESPONSE, status.error_code);
    return WebURLError(*status.blocked_by_response_reason,
                       status.resolve_error_info, has_copy_in_cache, url);
  }

  return WebURLError(status.error_code, status.extended_error_code,
                     status.resolve_error_info, has_copy_in_cache,
                     WebURLError::IsWebSecurityViolation::kFalse, url,
                     status.should_collapse_initiator
                         ? WebURLError::ShouldCollapseInitiator::kTrue
                         : WebURLError::ShouldCollapseInitiator::kFalse);
}

}  // namespace

// static
WebURLError WebURLError::Create(
    const network::URLLoaderCompletionStatus& status,
    const WebURL& url) {
  DCHECK_NE(net::OK, status.error_code);
  return CreateInternal(status, url);
}

WebURLError::WebURLError(int reason, const WebURL& url)
    : reason_(reason), url_(url) {
  DCHECK_NE(reason_, 0);
}

WebURLError::WebURLError(int reason,
                         int extended_reason,
                         net::ResolveErrorInfo resolve_error_info,
                         HasCopyInCache has_copy_in_cache,
                         IsWebSecurityViolation is_web_security_violation,
                         const WebURL& url,
                         ShouldCollapseInitiator should_collapse_initiator)
    : reason_(reason),
      extended_reason_(extended_reason),
      resolve_error_info_(resolve_error_info),
      has_copy_in_cache_(has_copy_in_cache == HasCopyInCache::kTrue),
      is_web_security_violation_(is_web_security_violation ==
                                 IsWebSecurityViolation::kTrue),
      url_(url),
      should_collapse_initiator_(should_collapse_initiator ==
                                 ShouldCollapseInitiator::kTrue) {
  DCHECK_NE(reason_, 0);
}

WebURLError::WebURLError(network::mojom::BlockedByResponseReason blocked_reason,
                         net::ResolveErrorInfo resolve_error_info,
                         HasCopyInCache has_copy_in_cache,
                         const WebURL& url)
    : reason_(net::ERR_BLOCKED_BY_RESPONSE),
      extended_reason_(0),
      resolve_error_info_(resolve_error_info),
      has_copy_in_cache_(has_copy_in_cache == HasCopyInCache::kTrue),
      is_web_security_violation_(false),
      url_(url),
      blocked_by_response_reason_(blocked_reason) {}

WebURLError::WebURLError(const network::CorsErrorStatus& cors_error_status,
                         HasCopyInCache has_copy_in_cache,
                         const WebURL& url)
    : reason_(net::ERR_FAILED),
      has_copy_in_cache_(has_copy_in_cache == HasCopyInCache::kTrue),
      is_web_security_violation_(true),
      url_(url),
      cors_error_status_(cors_error_status) {}

}  // namespace blink
