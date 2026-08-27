// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_CACHE_H_
#define SHOT_SHOT_CACHE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "base/types/expected.h"

namespace disk_cache {
class Backend;
}

namespace shot {

// One thing the cache is holding.
//
// `url` is the resource, recovered from the entry key. The key itself is not
// a URL -- HttpCache prefixes it with the network isolation partition, so it
// reads `_dk_https://example.com https://example.com/` -- and handing that
// out would make every caller write the same parser. See
// HttpCache::GetResourceURLFromHttpCacheKey.
struct CacheEntry {
  std::string url;
  base::Time last_used;
  // Headers and body together, which is what the entry costs on disk to
  // within a block.
  int64_t size = 0;

  // The backend's own identifier, kept because dooming an entry needs it and
  // the URL cannot be turned back into one: the partition prefix is dropped by
  // GetResourceURLFromHttpCacheKey and there is no inverse. Not reported to
  // callers -- it is a detail of how //net partitions its cache, and a caller
  // who matched on it would be matching on something that changes with the
  // isolation scheme.
  std::string key;
};

// Which entries to remove. All three filters are optional and they compose:
// nothing set means everything goes.
struct CacheClearOptions {
  // Exact resource URLs. The pattern matching a caller might want -- globs --
  // is deliberately not here: it belongs where the caller's own conventions
  // are, and doing it in the library would mean shipping a matcher whose
  // dialect nobody agreed on. The JS layer lists, matches, and passes the URLs
  // it decided on.
  std::vector<std::string> urls;
  // Entries not used since this moment. Note the direction: this is a
  // deadline, not an age, because an age would have to be resolved against a
  // clock and the caller already has one.
  std::optional<base::Time> unused_since;
  // Evict least-recently-used entries until the total is at or below this.
  std::optional<int64_t> max_bytes;
};

struct CacheClearResult {
  int removed = 0;
  int64_t bytes_before = 0;
  int64_t bytes_after = 0;
};

// Opens a cache directory for inspection.
//
// Within one process a directory has one backend: BackendCleanupTracker
// sequences them, so asking for a second while the engine holds the first does
// not fail -- it waits for the first to go away, which will not happen while
// the engine is up. That is the whole reason this is a separate entry point
// rather than something the engine always owns. Callers ask the engine for its
// backend in that case; see shot_api.cc, which is the one place that knows
// whether there is an engine.
//
// Across processes there is no lock at all, and two of them sharing a
// directory both get a working cache.
//
// Must be called on a thread with a task environment: the disk cache answers
// asynchronously and this waits for it on a nested run loop.
base::expected<std::unique_ptr<disk_cache::Backend>, std::string>
OpenCacheBackend(const base::FilePath& directory);

// Every entry, in whatever order the backend enumerates them.
//
// This opens each entry to read its key and size, which for the simple backend
// means touching every file. It is a diagnostic, not something to put on a
// request path.
base::expected<std::vector<CacheEntry>, std::string> ListCacheEntries(
    disk_cache::Backend* backend);

// Removes what `options` selects, through the backend rather than through the
// filesystem.
//
// Deleting the files directly would leave the simple backend's index naming
// entries that are no longer there, and the next process to open the directory
// rebuilds the index from disk -- or, having found it inconsistent, discards
// it. Dooming through the backend is the difference between clearing a cache
// and corrupting one.
base::expected<CacheClearResult, std::string> ClearCacheEntries(
    disk_cache::Backend* backend,
    const CacheClearOptions& options);

}  // namespace shot

#endif  // SHOT_SHOT_CACHE_H_
