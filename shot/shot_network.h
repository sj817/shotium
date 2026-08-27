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
class ShotNetwork {
 public:
  static base::expected<std::unique_ptr<ShotNetwork>, std::string> Create(
      const NetworkConfig& config);

  ShotNetwork(const ShotNetwork&) = delete;
  ShotNetwork& operator=(const ShotNetwork&) = delete;
  ~ShotNetwork();

  // The live context, or null when networking was never brought up. Global
  // because the thing that needs it -- ShotURLLoader -- is constructed by
  // blink's ResourceFetcher, several layers below anything that could have
  // been handed a pointer.
  static net::URLRequestContext* Get();

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

  std::unique_ptr<net::NetworkChangeNotifier> network_change_notifier_;
  std::unique_ptr<net::URLRequestContext> context_;
};

}  // namespace shot

#endif  // SHOT_SHOT_NETWORK_H_
