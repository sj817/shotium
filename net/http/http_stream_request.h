// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_HTTP_HTTP_STREAM_REQUEST_H_
#define NET_HTTP_HTTP_STREAM_REQUEST_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "net/base/load_states.h"
#include "net/base/load_timing_internal_info.h"
#include "net/base/net_error_details.h"
#include "net/base/net_export.h"
#include "net/base/request_priority.h"
#include "net/http/alternate_protocol_usage.h"
#include "net/http/alternative_service.h"
#include "net/http/http_response_info.h"
#include "net/log/net_log_source.h"
#include "net/log/net_log_with_source.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/socket/connection_attempts.h"
#include "net/socket/next_proto.h"
#include "net/spdy/spdy_session_key.h"
#include "net/spdy/spdy_session_pool.h"
#include "net/ssl/ssl_config.h"
#include "net/ssl/ssl_info.h"

namespace net {

class HttpAuthController;
class HttpStream;
class SSLCertRequestInfo;

// The HttpStreamRequest is the client's handle to the worker object which
// handles the creation of an HttpStream.  While the HttpStream is being
// created, this object is the creator's handle for interacting with the
// HttpStream creation process.  The request is cancelled by deleting it, after
// which no callbacks will be invoked.
class NET_EXPORT_PRIVATE HttpStreamRequest {
 public:
  // The HttpStreamRequest::Delegate is a set of callback methods for a
  // HttpStreamRequestJob.  Generally, only one of these methods will be
  // called as a result of a stream request.
  class NET_EXPORT_PRIVATE Delegate {
   public:
    virtual ~Delegate() = default;

    // This is the success case for RequestStream.
    // |stream| is now owned by the delegate.
    // |used_proxy_info| indicates the actual ProxyInfo used for this stream,
    // since the HttpStreamRequest performs the proxy resolution.
    virtual void OnStreamReady(const ProxyInfo& used_proxy_info,
                               std::unique_ptr<HttpStream> stream) = 0;

    // This is the failure to create a stream case.
    // |used_proxy_info| indicates the actual ProxyInfo used for this stream,
    // since the HttpStreamRequest performs the proxy resolution.
    virtual void OnStreamFailed(int status,
                                const NetErrorDetails& net_error_details,
                                const ProxyInfo& used_proxy_info,
                                ResolveErrorInfo resolve_error_info) = 0;

    // Called when we have a certificate error for the request.
    virtual void OnCertificateError(int status, const SSLInfo& ssl_info) = 0;

    // This is the failure for SSL Client Auth
    // Ownership of |cert_info| is retained by the HttpStreamRequest.  The
    // delegate may take a reference if it needs the cert_info beyond the
    // lifetime of this callback.
    virtual void OnNeedsClientAuth(SSLCertRequestInfo* cert_info) = 0;

    // Called when finding all QUIC alternative services are marked broken for
    // the origin in this request which advertises supporting QUIC.
  };

  class NET_EXPORT_PRIVATE Helper {
   public:
    virtual ~Helper() = default;

    // Returns the LoadState for Request.
    virtual LoadState GetLoadState() const = 0;

    // Called when Request is destructed.
    virtual void OnRequestComplete() = 0;

    // Called to resume the HttpStream creation process when necessary
    // Proxy authentication credentials are collected.

    // Called when the priority of transaction changes.
    virtual void SetPriority(RequestPriority priority) = 0;
  };

  struct CompletionDetails {
    // Protocol negotiated with the server.
    NextProto negotiated_protocol = NextProto::kProtoUnknown;
    // The reason why Chrome uses a specific transport protocol for HTTP
    // semantics.
    AlternateProtocolUsage alternate_protocol_usage =
        AlternateProtocolUsage::ALTERNATE_PROTOCOL_USAGE_UNSPECIFIED_REASON;
    // Indicates whether the request is used an existing H2 or H3 session.
    std::optional<SessionSource> session_source;
    // The state of the advertised alternative service.
    AdvertisedAltSvcState advertised_alt_svc_state;
  };

  // Request will notify `helper` when it's destructed.
  // Thus `helper` is valid for the lifetime of the `this` Request.
  HttpStreamRequest(Helper* helper, const NetLogWithSource& net_log);

  HttpStreamRequest(const HttpStreamRequest&) = delete;
  HttpStreamRequest& operator=(const HttpStreamRequest&) = delete;

  ~HttpStreamRequest();

  // Called when the priority of the parent transaction changes.
  void SetPriority(RequestPriority priority);

  // Marks completion of the request. Must be called before OnStreamReady().
  void Complete(CompletionDetails details);

  // Called by |helper_| to record connection attempts made by the socket
  // layer in an attached Job for this stream request.
  void AddConnectionAttempts(const ConnectionAttempts& attempts);

  // Returns the LoadState for the request.
  LoadState GetLoadState() const;

  // Protocol negotiated with the server.
  NextProto negotiated_protocol() const;

  // The reason why Chrome uses a specific transport protocol for HTTP
  // semantics.
  AlternateProtocolUsage alternate_protocol_usage() const;

  // Details of the completion of this request. Should be called after one
  // of the delegate callback methods. Returns std::nullopt when this didn't
  // complete successfully.
  const std::optional<CompletionDetails>& completion_details() const {
    return completion_details_;
  }

  // Returns socket-layer connection attempts made for this stream request.
  const ConnectionAttempts& connection_attempts() const;

  const NetLogWithSource& net_log() const { return net_log_; }

  bool completed() const { return completion_details_.has_value(); }

  void SetDnsResolutionTimeOverrides(
      base::TimeTicks dns_resolution_start_time_override,
      base::TimeTicks dns_resolution_end_time_override);

  base::TimeTicks dns_resolution_start_time_override() const {
    return dns_resolution_start_time_override_;
  }
  base::TimeTicks dns_resolution_end_time_override() const {
    return dns_resolution_end_time_override_;
  }

  // Sets a new helper for this request so that the new helper can take over
  // the responsibility of processing this request.
  //
  // This *MUST NOT* be used other than switching from HttpStreamFactory to
  // HttpStreamPool. (Re)setting the helper is extremetely dangerous and can
  // cause dangling pointers and/or UAFs very easily. This method only exists to
  // work around the fact that the HttpStreamFactory::JobController performs
  // proxy resolution for a request. Ideally we should separate proxy resolution
  // from HttpStreamFactory::JobController and use HttpStreamPool directly from
  // HttpNetworkTransaction, instead of setting the helper.
  //
  // TODO(crbug.com/346835898): Remove this method once we come up with a way
  // to separate proxy resolution from the HttpStreamFactory::JobController.
  void SetHelperForSwitchingToPool(Helper* helper);

 private:
  // Unowned. The helper must not be destroyed before this object is.
  raw_ptr<Helper> helper_;

  const NetLogWithSource net_log_;

  std::optional<CompletionDetails> completion_details_;
  ConnectionAttempts connection_attempts_;

  base::TimeTicks dns_resolution_start_time_override_;
  base::TimeTicks dns_resolution_end_time_override_;
};

}  // namespace net

#endif  // NET_HTTP_HTTP_STREAM_REQUEST_H_
