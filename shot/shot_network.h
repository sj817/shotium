// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_NETWORK_H_
#define SHOT_SHOT_NETWORK_H_

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/types/expected.h"

namespace disk_cache {
class Backend;
}

namespace net {
class NetworkChangeNotifier;
class URLRequestContext;
}  // namespace net

namespace shot {

// How the process-wide network stack is configured. Everything here is a
// process-level decision rather than a per-request one, which is why it comes
// off the command line and not out of a ScreenshotRequest.
struct NetworkConfig {
  // Where the HTTP cache lives. Empty disables caching entirely.
  //
  // Two processes may point at the same directory and both get a working
  // cache: the simple backend takes no cross-process lock, and each keeps its
  // own index. Measured, the second process reads what the first one wrote.
  // What they can disagree about is the index, and an index found
  // inconsistent is rebuilt from the entries, which carry checksums. Inside
  // one process it is genuinely exclusive -- see BackendCleanupTracker -- so
  // the engine's own backend has to be borrowed rather than reopened.
  base::FilePath cache_dir;

  // 0 lets the backend size itself from the free space on the volume.
  int cache_max_bytes = 0;

  // Empty means the built-in string.
  std::string user_agent;
};

// The process's one net::URLRequestContext.
//
// This is //net used directly -- URLRequest, HttpCache, HttpNetworkSession,
// BoringSSL -- with no //services/network above it. That service is the mojo
// wrapper multi-process Chrome puts around exactly these objects so that a
// sandboxed renderer can reach them; a shot worker is already its own process
// and owns its own stack, so the wrapper would be a pipe to itself.
//
// It must be created on, and used from, the thread that runs blink, and that
// thread must have an IO message pump: net watches sockets through
// base::CurrentIOThread.
//
// Brought up in two steps, because most of what it costs is paid for by
// captures that never use it. A file: capture touches nothing in //net beyond
// the MIME table, and the CLI's default is exactly that; yet building the
// stack was 60% of the engine's start, and 10 of those milliseconds were the
// network change notifier alone, which on Windows asks the OS for the
// connection type synchronously and starts a DNS-configuration watcher. So:
//
//  * Create() records the configuration and, when a cache directory is
//    configured, builds the context and opens the cache -- that is what makes
//    `cacheActive` answerable at start(), and a caller who configured a cache
//    has said they mean to fetch. Without one it builds nothing.
//  * EnsureUp() finishes the job on the first http(s) request: the context if
//    Create() left it, and the change notifier always. The notifier can come
//    last because //net's observer lists are process globals that exist
//    whether or not a notifier does; the resolver registered at build time
//    is notified by a notifier created afterwards. Until one exists the
//    connection type reads as CONNECTION_UNKNOWN, which //net's own
//    kDeferConnectionTypeAtStartup documents as "connected, type not yet
//    determined".
class ShotNetwork {
 public:
  static base::expected<std::unique_ptr<ShotNetwork>, std::string> Create(
      const NetworkConfig& config);

  ShotNetwork(const ShotNetwork&) = delete;
  ShotNetwork& operator=(const ShotNetwork&) = delete;
  ~ShotNetwork();

  // The live context, or null when it has not been built yet or networking
  // was never configured. Does not build it: this is the question "is there
  // one", and the callers that need one call EnsureUp(). Global because the
  // thing that needs it -- ShotURLLoader -- is constructed by blink's
  // ResourceFetcher, several layers below anything that could have been
  // handed a pointer.
  static net::URLRequestContext* Get();

  // The context, built now if it was not yet, with the change notifier up.
  // The error is the one Create() would have reported had it built eagerly.
  // Null with no error means there is no ShotNetwork in this process at all.
  static base::expected<net::URLRequestContext*, std::string> EnsureUp();

  // The User-Agent every request carries. Exposed because blink also reports it
  // to the document (navigator.userAgent, and the UA client hints), and the two
  // disagreeing is the kind of thing that makes a server serve one page and the
  // screenshot show another.
  static const std::string& UserAgent();

  // The cache this process is using, if it has one.
  //
  // Both of these exist for the same reason: a caller who wants to inspect or
  // clear this directory cannot open a second backend on it from inside this
  // process. disk_cache::BackendCleanupTracker sequences backends per
  // directory within a process, so the attempt does not fail -- it waits for
  // the first one to go away, which is not going to happen while the engine
  // is up. Asking the engine for the backend it already has is the way in,
  // and comparing the directory is how a caller finds out whether that is the
  // situation it is in. Empty and null respectively when caching was never
  // configured.
  static const base::FilePath& CacheDir();
  static disk_cache::Backend* CacheBackend();

  // Whether the directory above is actually being cached into.
  //
  // A configured directory that could not be opened -- no permission, no
  // space, a path that is not a directory -- otherwise fails invisibly: the
  // engine renders correctly and every capture pays for the network again.
  // Create() opens the cache eagerly so that this is answerable before the
  // first screenshot rather than after it.
  static bool CacheActive();

 private:
  ShotNetwork();

  // Builds the context from `config_`. Idempotent.
  base::expected<void, std::string> BuildContext();

  NetworkConfig config_;
  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
  std::unique_ptr<net::URLRequestContext> context_;
};

}  // namespace shot

#endif  // SHOT_SHOT_NETWORK_H_
