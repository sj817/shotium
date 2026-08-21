// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/http_network_session.h"

#include <inttypes.h>

#include <utility>

#include "base/atomic_sequence_num.h"
#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/feature_list.h"
#include "base/notreached.h"
#include "base/power_monitor/power_monitor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "build/build_config.h"
#include "net/base/features.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_response_body_drainer.h"
#include "net/http/http_stream_factory.h"
#include "net/http/http_stream_pool.h"
#include "net/http/url_security_manager.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/socket/client_socket_factory.h"
#include "net/socket/client_socket_pool_manager_impl.h"
#include "net/socket/next_proto.h"
#include "net/socket/ssl_client_socket.h"
#include "net/socket/stream_socket_close_reason.h"
#include "net/spdy/spdy_session.h"
#include "net/spdy/spdy_session_pool.h"
#include "url/scheme_host_port.h"

namespace net {

// The maximum receive window sizes for HTTP/2 sessions and streams.
const int32_t kSpdySessionMaxRecvWindowSize = 15 * 1024 * 1024;  // 15 MB
const int32_t kSpdyStreamMaxRecvWindowSize = 6 * 1024 * 1024;    //  6 MB

// Value of SETTINGS_ENABLE_PUSH reflecting that server push is not supported.
const uint32_t kSpdyDisablePush = 0;

namespace {

// Keep all HTTP2 parameters in |http2_settings|, even the ones that are not
// implemented, to be sent to the server.
// Set default values for settings that |http2_settings| does not specify.
spdy::SettingsMap AddDefaultHttp2Settings(spdy::SettingsMap http2_settings) {
  // Server push is not supported.
  http2_settings[spdy::SETTINGS_ENABLE_PUSH] = kSpdyDisablePush;

  // For other setting parameters, set default values only if |http2_settings|
  // does not have a value set for given setting.
  auto it = http2_settings.find(spdy::SETTINGS_HEADER_TABLE_SIZE);
  if (it == http2_settings.end()) {
    http2_settings[spdy::SETTINGS_HEADER_TABLE_SIZE] = kSpdyMaxHeaderTableSize;
  }

  it = http2_settings.find(spdy::SETTINGS_INITIAL_WINDOW_SIZE);
  if (it == http2_settings.end()) {
    http2_settings[spdy::SETTINGS_INITIAL_WINDOW_SIZE] =
        kSpdyStreamMaxRecvWindowSize;
  }

  it = http2_settings.find(spdy::SETTINGS_MAX_HEADER_LIST_SIZE);
  if (it == http2_settings.end()) {
    http2_settings[spdy::SETTINGS_MAX_HEADER_LIST_SIZE] =
        kSpdyMaxHeaderListSize;
  }

  return http2_settings;
}

}  // unnamed namespace

HttpNetworkSessionParams::HttpNetworkSessionParams()
    : spdy_session_max_recv_window_size(kSpdySessionMaxRecvWindowSize),
      spdy_session_max_queued_capped_frames(kSpdySessionMaxQueuedCappedFrames),
      time_func(&base::TimeTicks::Now) {
  enable_early_data =
      base::FeatureList::IsEnabled(features::kEnableTLS13EarlyData);
}

HttpNetworkSessionParams::HttpNetworkSessionParams(
    const HttpNetworkSessionParams& other) = default;

HttpNetworkSessionParams::~HttpNetworkSessionParams() = default;

HttpNetworkSessionContext::HttpNetworkSessionContext()
    : client_socket_factory(nullptr),
      host_resolver(nullptr),
      cert_verifier(nullptr),
      transport_security_state(nullptr),
      sct_auditing_delegate(nullptr),
      proxy_resolution_service(nullptr),
      proxy_delegate(nullptr),
      http_user_agent_settings(nullptr),
      ssl_config_service(nullptr),
      http_auth_handler_factory(nullptr),
      net_log(nullptr),
      socket_performance_watcher_factory(nullptr),
      network_quality_estimator(nullptr)
#if BUILDFLAG(ENABLE_REPORTING)
      ,
      reporting_service(nullptr),
      network_error_logging_service(nullptr)
#endif
{
}

HttpNetworkSessionContext::HttpNetworkSessionContext(
    const HttpNetworkSessionContext& other) = default;

HttpNetworkSessionContext::~HttpNetworkSessionContext() = default;

// TODO(mbelshe): Move the socket factories into HttpStreamFactory.
HttpNetworkSession::HttpNetworkSession(const HttpNetworkSessionParams& params,
                                       const HttpNetworkSessionContext& context)
    : net_log_(context.net_log),
      http_server_properties_(context.http_server_properties),
      cert_verifier_(context.cert_verifier),
      http_auth_handler_factory_(context.http_auth_handler_factory),
      host_resolver_(context.host_resolver),
#if BUILDFLAG(ENABLE_REPORTING)
      reporting_service_(context.reporting_service),
      network_error_logging_service_(context.network_error_logging_service),
#endif
      proxy_resolution_service_(context.proxy_resolution_service),
      ssl_config_service_(context.ssl_config_service),
      http_auth_cache_(
          params.key_auth_cache_server_entries_by_network_anonymization_key),
      ssl_client_session_cache_(SSLClientSessionCache::Config()),
      ssl_client_context_(context.ssl_config_service,
                          context.cert_verifier,
                          context.transport_security_state,
                          &ssl_client_session_cache_,
                          context.sct_auditing_delegate),
      spdy_session_pool_(context.host_resolver,
                         &ssl_client_context_,
                         context.http_server_properties,
                         context.transport_security_state,
                         params.enable_spdy_ping_based_connection_checking,
                         params.enable_http2,
                         params.spdy_session_max_recv_window_size,
                         params.spdy_session_max_queued_capped_frames,
                         AddDefaultHttp2Settings(params.http2_settings),
                         params.enable_http2_settings_grease,
                         params.greased_http2_frame,
                         params.http2_end_stream_with_data_frame,
                         params.enable_priority_update,
                         params.spdy_go_away_on_ip_change,
                         params.time_func,
                         context.network_quality_estimator,
                         // cleanup_sessions_on_ip_address_changed
                         !params.ignore_ip_address_changes),
      http_stream_factory_(std::make_unique<HttpStreamFactory>(this)),
      params_(params),
      context_(context) {
  DCHECK(proxy_resolution_service_);
  DCHECK(ssl_config_service_);
  CHECK(http_server_properties_);
  DCHECK(context_.client_socket_factory);

  normal_socket_pool_manager_ = std::make_unique<ClientSocketPoolManagerImpl>(
      CreateCommonConnectJobParams(false /* for_websockets */),
      CreateCommonConnectJobParams(true /* for_websockets */),
      SocketPoolType::kNormal,
      // cleanup_on_ip_address_change
      !params.ignore_ip_address_changes);
  websocket_socket_pool_manager_ =
      std::make_unique<ClientSocketPoolManagerImpl>(
          CreateCommonConnectJobParams(false /* for_websockets */),
          CreateCommonConnectJobParams(true /* for_websockets */),
          SocketPoolType::kWebSocket,
          // cleanup_on_ip_address_change
          !params.ignore_ip_address_changes);

  if (params_.enable_http2) {
    next_protos_.push_back(NextProto::kProtoHTTP2);
    if (base::FeatureList::IsEnabled(features::kAlpsForHttp2)) {
      // Enable ALPS for HTTP/2 with empty data.
      application_settings_[NextProto::kProtoHTTP2] = {};
    }
  }

  next_protos_.push_back(NextProto::kProtoHTTP11);

  http_stream_pool_ = std::make_unique<HttpStreamPool>(
      this,
      /*cleanup_on_ip_address_change=*/!params.ignore_ip_address_changes);
#if BUILDFLAG(IS_WIN)
  base::PowerMonitor::GetInstance()->AddPowerSuspendObserver(this);
#endif
}

HttpNetworkSession::~HttpNetworkSession() {
  DCHECK_CALLED_ON_VALID_THREAD(thread_checker_);
#if BUILDFLAG(IS_WIN)
  base::PowerMonitor::GetInstance()->RemovePowerSuspendObserver(this);
#endif
  if (http_stream_pool_) {
    http_stream_pool_->OnShuttingDown();
  }
  response_drainers_.clear();
  // TODO(bnc): CloseAllSessions() is also called in SpdySessionPool destructor,
  // one of the two calls should be removed.
  spdy_session_pool_.CloseAllSessions();
}

void HttpNetworkSession::StartResponseDrainer(
    std::unique_ptr<HttpResponseBodyDrainer> drainer) {
  DCHECK(!response_drainers_.contains(drainer.get()));
  HttpResponseBodyDrainer* drainer_ptr = drainer.get();
  response_drainers_.insert(std::move(drainer));
  drainer_ptr->Start(this);
}

void HttpNetworkSession::RemoveResponseDrainer(
    HttpResponseBodyDrainer* drainer) {
  DCHECK(response_drainers_.contains(drainer));

  response_drainers_.erase(response_drainers_.find(drainer));
}

ClientSocketPool* HttpNetworkSession::GetSocketPool(
    SocketPoolType pool_type,
    const ProxyChain& proxy_chain) {
  return GetSocketPoolManager(pool_type)->GetSocketPool(proxy_chain);
}

base::Value HttpNetworkSession::SocketPoolInfoToValue() const {
  // TODO(yutak): Should merge values from normal pools and WebSocket pools.
  return normal_socket_pool_manager_->SocketPoolInfoToValue();
}

base::Value HttpNetworkSession::SpdySessionPoolInfoToValue() const {
  return spdy_session_pool_.SpdySessionPoolInfoToValue();
}

void HttpNetworkSession::CloseAllConnections(int net_error,
                                             const char* net_log_reason_utf8) {
  normal_socket_pool_manager_->FlushSocketPoolsWithError(net_error,
                                                         net_log_reason_utf8);
  websocket_socket_pool_manager_->FlushSocketPoolsWithError(
      net_error, net_log_reason_utf8);
  if (http_stream_pool_) {
    http_stream_pool_->FlushWithError(
        net_error, StreamSocketCloseReason::kCloseAllConnections,
        net_log_reason_utf8);
  }
  spdy_session_pool_.CloseCurrentSessions(static_cast<Error>(net_error));
}

void HttpNetworkSession::CloseIdleConnections(const char* net_log_reason_utf8) {
  normal_socket_pool_manager_->CloseIdleSockets(net_log_reason_utf8);
  websocket_socket_pool_manager_->CloseIdleSockets(net_log_reason_utf8);
  if (http_stream_pool_) {
    http_stream_pool_->CloseIdleStreams(net_log_reason_utf8);
  }
  spdy_session_pool_.CloseCurrentIdleSessions(net_log_reason_utf8);
}

void HttpNetworkSession::SetTLS13EarlyDataEnabled(bool enabled) {
  params_.enable_early_data = enabled;
}

void HttpNetworkSession::IgnoreCertificateErrorsForTesting() {
  params_.ignore_certificate_errors = true;
}

void HttpNetworkSession::ClearSSLSessionCache() {
  ssl_client_session_cache_.Flush();
}

CommonConnectJobParams HttpNetworkSession::CreateCommonConnectJobParams(
    bool for_websockets) {
  // Use null websocket_endpoint_lock_manager, which is only set for WebSockets,
  // and only when not using a proxy.
  return CommonConnectJobParams(
      context_.client_socket_factory, context_.host_resolver, &http_auth_cache_,
      context_.http_auth_handler_factory, &spdy_session_pool_,
      context_.proxy_delegate, context_.http_user_agent_settings,
      &ssl_client_context_, context_.socket_performance_watcher_factory,
      context_.network_quality_estimator, context_.net_log,
      for_websockets ? &websocket_endpoint_lock_manager_ : nullptr,
      context_.http_server_properties, &next_protos_, &application_settings_,
      &params_.ignore_certificate_errors, &params_.enable_early_data);
}

void HttpNetworkSession::ApplyTestingFixedPort(
    url::SchemeHostPort& endpoint) const {
  bool using_ssl = GURL::SchemeIsCryptographic(endpoint.scheme());
  if (!using_ssl && params().testing_fixed_http_port != 0) {
    endpoint = url::SchemeHostPort(endpoint.scheme(), endpoint.host(),
                                   params().testing_fixed_http_port);
  } else if (using_ssl && params().testing_fixed_https_port != 0) {
    endpoint = url::SchemeHostPort(endpoint.scheme(), endpoint.host(),
                                   params().testing_fixed_https_port);
  }
}

ClientSocketPoolManager* HttpNetworkSession::GetSocketPoolManager(
    SocketPoolType pool_type) {
  switch (pool_type) {
    case SocketPoolType::kNormal:
      return normal_socket_pool_manager_.get();
    case SocketPoolType::kWebSocket:
      return websocket_socket_pool_manager_.get();
  }
  NOTREACHED();
}

void HttpNetworkSession::OnSuspend() {
  power_suspended_ = true;
  CloseIdleConnections("Entering suspend mode");
}

void HttpNetworkSession::OnResume() {
  power_suspended_ = false;
}

}  // namespace net
