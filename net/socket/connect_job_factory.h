// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_SOCKET_CONNECT_JOB_FACTORY_H_
#define NET_SOCKET_CONNECT_JOB_FACTORY_H_

#include <memory>
#include <vector>

#include "net/base/network_anonymization_key.h"
#include "net/base/network_handle.h"
#include "net/base/privacy_mode.h"
#include "net/base/request_priority.h"
#include "net/dns/public/secure_dns_policy.h"
#include "net/socket/connect_job.h"
#include "net/socket/socket_tag.h"
#include "net/socket/ssl_connect_job.h"
#include "net/socket/transport_connect_job.h"
#include "net/ssl/ssl_config.h"
#include "url/scheme_host_port.h"

namespace net {

class NetworkAnonymizationKey;
struct NetworkTrafficAnnotationTag;
struct SSLConfig;

// Common factory for transport and TLS connections.
class NET_EXPORT_PRIVATE ConnectJobFactory {
 public:
  // Default factory will be used if passed the default `nullptr`.
  explicit ConnectJobFactory(
      std::unique_ptr<SSLConnectJob::Factory> ssl_connect_job_factory = nullptr,
      std::unique_ptr<TransportConnectJob::Factory>
          transport_connect_job_factory = nullptr);

  // Not copyable/movable. Intended for polymorphic use via pointer.
  ConnectJobFactory(const ConnectJobFactory&) = delete;
  ConnectJobFactory& operator=(const ConnectJobFactory&) = delete;

  virtual ~ConnectJobFactory();

  // `common_connect_job_params` and `delegate` must outlive the returned
  // ConnectJob.
  virtual std::unique_ptr<ConnectJob> CreateConnectJob(
      url::SchemeHostPort endpoint,
      const std::vector<SSLConfig::CertAndStatus>& allowed_bad_certs,
      PrivacyMode privacy_mode,
      const OnHostResolutionCallback& resolution_callback,
      RequestPriority request_priority,
      SocketTag socket_tag,
      const NetworkAnonymizationKey& network_anonymization_key,
      SecureDnsPolicy secure_dns_policy,
      bool disable_cert_network_fetches,
      const CommonConnectJobParams* common_connect_job_params,
      handles::NetworkHandle target_network,
      ConnectJob::Delegate* delegate) const;

 private:
  std::unique_ptr<SSLConnectJob::Factory> ssl_connect_job_factory_;
  std::unique_ptr<TransportConnectJob::Factory> transport_connect_job_factory_;
};

}  // namespace net

#endif  // NET_SOCKET_CONNECT_JOB_FACTORY_H_
