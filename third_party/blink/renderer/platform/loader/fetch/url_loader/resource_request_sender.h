// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_URL_LOADER_RESOURCE_REQUEST_SENDER_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_URL_LOADER_RESOURCE_REQUEST_SENDER_H_

#include "net/base/load_timing_info.h"
#include <stdint.h>

#include <memory>
#include <string>
#include <vector>

#include "base/byte_size.h"
#include "base/functional/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/task/sequenced_task_runner.h"
#include "base/time/time.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "net/base/host_port_pair.h"
#include "net/base/net_errors.h"
#include "net/base/request_priority.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/http_request_headers_update_params.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/fetch_api.mojom-forward.h"
#include "services/network/public/mojom/url_response_head.mojom-forward.h"
#include "third_party/blink/public/common/loader/url_loader_throttle.h"
#include "third_party/blink/public/mojom/blob/blob_registry.mojom-blink.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"
#include "third_party/blink/public/mojom/navigation/renderer_eviction_reason.mojom-blink-forward.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/renderer/platform/allow_discouraged_type.h"
#include "third_party/blink/renderer/platform/loader/fetch/loader_freeze_mode.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace base {
class WaitableEvent;
}

namespace net {
struct RedirectInfo;
}

namespace network {
struct ResourceRequest;
struct URLLoaderCompletionStatus;
}  // namespace network

namespace blink {
class ThrottlingURLLoader;
class MojoURLLoaderClient;
class ResourceRequestClient;
struct SyncLoadResponse;

// This class creates a PendingRequestInfo object and handles sending a resource
// request asynchronously or synchronously, and it's owned by
// URLLoader::Context or SyncLoadContext.
class BLINK_PLATFORM_EXPORT ResourceRequestSender {
 public:
  ResourceRequestSender();
  ResourceRequestSender(const ResourceRequestSender&) = delete;
  ResourceRequestSender& operator=(const ResourceRequestSender&) = delete;
  virtual ~ResourceRequestSender();

  // Call this method to load the resource synchronously (i.e., in one shot).
  // This is an alternative to the StartAsync method. Be warned that this method
  // will block the calling thread until the resource is fully downloaded or an
  // error occurs. It could block the calling thread for a long time, so only
  // use this if you really need it!  There is also no way for the caller to
  // interrupt this method. Errors are reported via the status field of the
  // response parameter.
  //
  // |timeout| is used to abort the sync request on timeouts. TimeDelta::Max()
  // is interpreted as no-timeout.
  // If |download_to_blob_registry| is not null, it is used to redirect the
  // download to a blob.
  virtual void SendSync(
      std::unique_ptr<network::ResourceRequest> request,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      uint32_t loader_options,
      SyncLoadResponse* response,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
      base::TimeDelta timeout,
      const Vector<String>& cors_exempt_header_list,
      base::WaitableEvent* terminate_sync_load_event,
      mojo::PendingRemote<mojom::blink::BlobRegistry> download_to_blob_registry,
      scoped_refptr<ResourceRequestClient> client);

  // Call this method to initiate the request. If this method succeeds, then
  // the client's methods will be called asynchronously to report various
  // events. Returns the request id. |url_loader_factory| must be non-null.
  //
  // You need to pass a non-null |loading_task_runner| to specify task queue to
  // execute loading tasks on.
  virtual int SendAsync(
      std::unique_ptr<network::ResourceRequest> request,
      scoped_refptr<base::SequencedTaskRunner> loading_task_runner,
      const net::NetworkTrafficAnnotationTag& traffic_annotation,
      uint32_t loader_options,
      const Vector<String>& cors_exempt_header_list,
      scoped_refptr<ResourceRequestClient> client,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::vector<std::unique_ptr<URLLoaderThrottle>> throttles,
      base::OnceCallback<void(mojom::blink::RendererEvictionReason)>
          evict_from_bfcache_callback,
      base::RepeatingCallback<void(size_t)>
          did_buffer_load_while_in_bfcache_callback);

  // Cancels the current request and `request_info_` will be released.
  virtual void Cancel(scoped_refptr<base::SequencedTaskRunner> task_runner);

  // Freezes the loader. See blink/renderer/platform/loader/README.md for the
  // general concept of "freezing" in the loading module. See
  // blink/public/platform/web_loader_freezing_mode.h for `mode`.
  virtual void Freeze(LoaderFreezeMode mode);

  // Indicates the priority of the specified request changed.
  void DidChangePriority(net::RequestPriority new_priority,
                         int intra_priority_value);

  virtual void DeletePendingRequest(
      scoped_refptr<base::SequencedTaskRunner> task_runner);

  // Called when the transfer size is updated.
  virtual void OnTransferSizeUpdated(base::ByteSize transfer_size_diff);

  // Called as upload progress is made.
  virtual void OnUploadProgress(int64_t position, int64_t size);

  // Called when response headers are available.
  virtual void OnReceivedResponse(
      network::mojom::URLResponseHeadPtr response_head,
      mojo::ScopedDataPipeConsumerHandle body,
      base::TimeTicks response_ipc_arrival_time);

  // Called when a redirect occurs.
  virtual void OnReceivedRedirect(
      const net::RedirectInfo& redirect_info,
      network::mojom::URLResponseHeadPtr response_head,
      base::TimeTicks redirect_ipc_arrival_time);

  // Called when the response is complete.
  virtual void OnRequestComplete(
      const network::URLLoaderCompletionStatus& status,
      base::TimeTicks complete_ipc_arrival_time);

 private:
  friend class URLLoaderClientImpl;
  friend class URLResponseBodyConsumer;

  struct PendingRequestInfo {
    PendingRequestInfo(scoped_refptr<ResourceRequestClient> client,
                       const KURL& request_url);

    ~PendingRequestInfo();

    scoped_refptr<ResourceRequestClient> client;
    LoaderFreezeMode freeze_mode = LoaderFreezeMode::kNone;
    // Original requested url.
    KURL url;
    // The url, method and referrer of the latest response even in case of
    // redirection.
    KURL response_url;
    base::TimeTicks local_request_start;
    base::TimeTicks local_response_start;
    base::TimeTicks remote_request_start;
    net::LoadTimingInfo load_timing_info;
    bool redirect_requires_loader_restart = false;
    // Network error code the request completed with, or net::ERR_IO_PENDING if
    // it's not completed. Used to distinguish completion from cancellation.
    int net_error = net::ERR_IO_PENDING;

    std::unique_ptr<ThrottlingURLLoader> url_loader;
    std::unique_ptr<MojoURLLoaderClient> url_loader_client;

    // The pending redirect information (`has_pending_redirect` and headers)
    // is set in `ResourceRequestSender::OnFollowRedirectCallback()` and is
    // used/cleared in `ResourceRequestSender::FollowPendingRedirect()`.
    bool has_pending_redirect = false;

    // Headers that need to be added or updated upon the pending redirect.
    // Used for Client Hints headers, `Shared-Storage-Writable` header in the
    // case that permission has been restored/revoked on a redirect, while this
    // comment might be outdated.
    network::HttpRequestHeadersUpdateParams headers_update_params;

  };

  // Called as a callback for ResourceRequestClient::OnReceivedRedirect().
  void OnFollowRedirectCallback(
      const net::RedirectInfo& redirect_info,
      std::vector<std::string> removed_headers,
      net::HttpRequestHeaders modified_headers);

  // Follows redirect, if any, for the given request.
  void FollowPendingRedirect();

  // Converts remote times in the response head to local times.
  void ToLocalURLResponseHead(
      const PendingRequestInfo& request_info,
      network::mojom::URLResponseHead& response_head) const;

  // The instance is created on StartAsync() or StartSync(), and it's deleted
  // when the response has finished, or when the request is canceled.
  std::unique_ptr<PendingRequestInfo> request_info_;

  // Set to true when OnReceivedResponse of the client is called.
  // This is used to prevent OnReceivedResponse from being called twice.
  bool response_sent_to_client_ = false;

  scoped_refptr<base::SequencedTaskRunner> loading_task_runner_;

  base::WeakPtrFactory<ResourceRequestSender> weak_factory_{this};
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_LOADER_FETCH_URL_LOADER_RESOURCE_REQUEST_SENDER_H_
