// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_network.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/run_loop.h"
#include "net/base/net_errors.h"
#include "net/base/network_change_notifier.h"
#include "net/http/http_cache.h"
#include "net/http/http_transaction_factory.h"
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

// Recorded rather than re-derived: the context knows it has a cache but not
// where it was told to put one, and a caller asking "is this the directory the
// engine has open?" is asking about what was configured.
base::FilePath& MutableCacheDir() {
  static base::NoDestructor<base::FilePath> cache_dir;
  return *cache_dir;
}

// Whether the directory in MutableCacheDir() is actually being cached into.
// False with a directory set means the lock was held by somebody else.
bool g_cache_active = false;

// Builds the HttpCache's backend now and reports whether it worked.
//
// The run loop is nestable because this is called during engine startup, from
// a thread whose own loop has not begun: the scheduler and its task queues
// exist by this point, so a nested loop turns, but there is no outer loop that
// would otherwise deliver the callback.
bool OpenCacheEagerly(net::URLRequestContext* context) {
  net::HttpTransactionFactory* factory = context->http_transaction_factory();
  if (!factory) {
    return false;
  }
  net::HttpCache* cache = factory->GetCache();
  if (!cache) {
    return false;
  }

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  net::HttpCache::GetBackendResult result{net::ERR_IO_PENDING, nullptr};
  bool answered = false;

  net::HttpCache::GetBackendResult immediate = cache->GetBackend(base::BindOnce(
      [](base::RunLoop* loop, net::HttpCache::GetBackendResult* out,
         bool* answered, net::HttpCache::GetBackendResult value) {
        *out = value;
        *answered = true;
        loop->Quit();
      },
      &run_loop, &result, &answered));

  if (immediate.first != net::ERR_IO_PENDING) {
    result = immediate;
  } else if (!answered) {
    run_loop.Run();
  }

  if (result.first != net::OK || !result.second) {
    LOG(WARNING) << "shot: the cache directory could not be opened ("
                 << net::ErrorToShortString(
                        static_cast<net::Error>(result.first))
                 << "); this process is running without a cache.";
    return false;
  }
  return true;
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
  MutableCacheDir().clear();
  g_cache_active = false;
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

  // HTTP/2 on. HTTP/3 is no longer a runtime choice: //net/quic and quiche's
  // quic/ tree are out of this build, so there is nothing left to enable. A
  // server that advertises h3 over Alt-Svc is simply not followed, and the
  // request stays on h2 or HTTP/1.1.
  builder.SetSpdyEnabled(/*spdy_enabled=*/true);

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
    MutableCacheDir() = config.cache_dir;
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

  // The cache is opened here rather than on the first request that wants it.
  //
  // HttpCache builds its backend lazily, which means "did the cache actually
  // work" is not knowable until a screenshot has already been taken -- and the
  // answer can be no, silently. A directory that cannot be created, cannot be
  // written to, or is not a directory at all costs nothing visible: the engine
  // renders exactly as well without a cache, only slower, and every capture
  // pays the network again for a reason nothing reports.
  //
  // That was survivable while caching was off by default and everyone using it
  // had named a directory on purpose. With a default directory it is worth
  // one open at startup -- where the caller already agreed to pay for the
  // engine coming up -- so that start() can return the answer.
  if (!config.cache_dir.empty()) {
    g_cache_active = OpenCacheEagerly(network->context_.get());
  }
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

// static
const base::FilePath& ShotNetwork::CacheDir() {
  return MutableCacheDir();
}

// static
bool ShotNetwork::CacheActive() {
  return g_cache_active;
}

// static
disk_cache::Backend* ShotNetwork::CacheBackend() {
  if (!g_context) {
    return nullptr;
  }
  net::HttpTransactionFactory* factory = g_context->http_transaction_factory();
  if (!factory) {
    return nullptr;
  }
  net::HttpCache* cache = factory->GetCache();
  if (!cache) {
    return nullptr;
  }
  // GetCurrentBackend and not GetBackend: the asynchronous one would create
  // the backend if it did not exist yet, and a caller who only wants to look
  // at a cache should not be the reason one comes into being. Null here means
  // the engine has not needed its cache yet, which is a true answer.
  return cache->GetCurrentBackend();
}

}  // namespace shot
