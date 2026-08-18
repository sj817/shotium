// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_network.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "net/base/network_change_notifier.h"
#include "net/proxy_resolution/configured_proxy_resolution_service.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"

namespace shot {
namespace {

// The one context, and the one User-Agent. Not owned here: ShotNetwork owns
// both and clears these on the way out, so a use after teardown is a null
// pointer rather than a dangling one.
net::URLRequestContext* g_context = nullptr;

std::string& MutableUserAgent() {
  static base::NoDestructor<std::string> user_agent;
  return *user_agent;
}

// Chrome's reduced User-Agent, with this tree's milestone in it.
//
// Reduced means the minor/build/patch fields are frozen at 0 upstream too, so
// there is nothing here that a real Chrome would report differently -- and
// nothing that identifies the host. The milestone is written out rather than
// read from chrome/VERSION because //chrome is not in this binary's dependency
// graph at all; docs/cut-progress.md has the version this tree was cut from.
constexpr char kDefaultUserAgent[] =
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, "
    "like Gecko) Chrome/153.0.0.0 Safari/537.36";

}  // namespace

ShotNetwork::ShotNetwork() = default;

ShotNetwork::~ShotNetwork() {
  g_context = nullptr;
  MutableUserAgent().clear();
}

// static
base::expected<std::unique_ptr<ShotNetwork>, std::string> ShotNetwork::Create(
    const NetworkConfig& config) {
  if (g_context) {
    return base::unexpected("the network stack is already up");
  }

  std::unique_ptr<ShotNetwork> network(new ShotNetwork());

  // HostResolverManager registers as an IP-address and connection-type
  // observer the moment it is built, and the answers it gets without a
  // notifier are "unknown" forever -- which means a resolver that never
  // flushes its cache when the machine changes networks. A resident worker can
  // outlive a network change, so it gets a real notifier.
  network->network_change_notifier_ = net::NetworkChangeNotifier::CreateIfNeeded();

  MutableUserAgent() =
      config.user_agent.empty() ? kDefaultUserAgent : config.user_agent;

  net::URLRequestContextBuilder builder;
  builder.set_user_agent(MutableUserAgent());
  builder.set_accept_language("en-US,en;q=0.9");

  // Direct, always. Honouring the system proxy means PAC and WPAD -- a script
  // interpreter and a discovery protocol -- and this binary has no script
  // interpreter left to run a PAC file in.
  builder.set_proxy_resolution_service(
      net::ConfiguredProxyResolutionService::CreateDirect());

  // HTTP/2 on, HTTP/3 off. h2 is already linked in through
  // net/spdy/spdy_session.cc and costs nothing to allow; QUIC would drag in
  // quiche for a protocol whose win -- connection setup latency across many
  // origins -- barely shows up when a screenshot fetches a handful of
  // subresources from one or two hosts.
  builder.SetSpdyAndQuicEnabled(/*spdy_enabled=*/true, /*quic_enabled=*/false);

  // Real servers negotiate brotli; asking for it and then not being able to
  // decode it is worse than not asking. //third_party/brotli is in the tree.
  builder.set_enable_brotli(true);

  if (config.cache_dir.empty()) {
    builder.DisableHttpCache();
  } else {
    if (!base::CreateDirectory(config.cache_dir)) {
      return base::unexpected("could not create the cache directory " +
                              config.cache_dir.AsUTF8Unsafe());
    }
    net::URLRequestContextBuilder::HttpCacheParams params;
    // Simple, explicitly, rather than DISK's "default backend". One file per
    // entry, no database: net/disk_cache/simple/ has no sqlite reference in it,
    // so a disk cache here does not undo enable_disk_cache_sql_backend=false.
    params.type = net::URLRequestContextBuilder::HttpCacheParams::DISK_SIMPLE;
    params.path = config.cache_dir;
    params.max_size = config.cache_max_bytes;
    builder.EnableHttpCache(params);
  }

  // Everything not named above is the builder's default, and the defaults are
  // the ones a browser uses: an in-memory CookieMonster (a redirect chain that
  // sets a cookie and expects it back is ordinary, even with no script to read
  // it), CertVerifier::CreateDefault over the platform trust store,
  // TransportSecurityState with the preloaded HSTS list, and the system DNS
  // resolver.
  network->context_ = builder.Build();
  if (!network->context_) {
    return base::unexpected("could not build the URLRequestContext");
  }
  g_context = network->context_.get();
  return network;
}

// static
net::URLRequestContext* ShotNetwork::Get() {
  return g_context;
}

// static
const std::string& ShotNetwork::UserAgent() {
  return MutableUserAgent();
}

}  // namespace shot
