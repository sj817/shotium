// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_cache.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/functional/bind.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "net/base/cache_type.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"
#include "net/http/http_cache.h"

namespace shot {
namespace {

// The disk cache answers either inline or through a callback, and every call
// site below wants the answer before it continues. These three are that
// pattern, once per result shape: run the operation with a callback that quits
// a nested loop, and if it answered inline, do not run the loop at all.
//
// Nestable because the caller may already be inside a run loop -- the engine
// thread always is -- and a cache operation posted from there has to make
// progress without waiting for the outer loop to finish, which it never will.
//
// One template covering all three would still have to be told how each
// recognises "pending", and they do not agree: int64_t results use
// ERR_IO_PENDING as an ordinary value in the same range as the answer.
int WaitForCompletion(
    base::OnceCallback<int(net::CompletionOnceCallback)> start) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  int result = net::ERR_IO_PENDING;
  int immediate = std::move(start).Run(base::BindOnce(
      [](base::RunLoop* loop, int* out, int value) {
        *out = value;
        loop->Quit();
      },
      &run_loop, &result));
  if (immediate != net::ERR_IO_PENDING) {
    return immediate;
  }
  run_loop.Run();
  return result;
}

int64_t WaitForInt64(
    base::OnceCallback<int64_t(net::Int64CompletionOnceCallback)> start) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  int64_t result = net::ERR_IO_PENDING;
  int64_t immediate = std::move(start).Run(base::BindOnce(
      [](base::RunLoop* loop, int64_t* out, int64_t value) {
        *out = value;
        loop->Quit();
      },
      &run_loop, &result));
  if (immediate != net::ERR_IO_PENDING) {
    return immediate;
  }
  run_loop.Run();
  return result;
}

disk_cache::EntryResult WaitForEntry(
    base::OnceCallback<disk_cache::EntryResult(disk_cache::EntryResultCallback)>
        start) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  disk_cache::EntryResult result;
  bool answered = false;
  disk_cache::EntryResult immediate = std::move(start).Run(base::BindOnce(
      [](base::RunLoop* loop, disk_cache::EntryResult* out, bool* answered,
         disk_cache::EntryResult value) {
        *out = std::move(value);
        *answered = true;
        loop->Quit();
      },
      &run_loop, &result, &answered));
  if (immediate.net_error() != net::ERR_IO_PENDING) {
    return immediate;
  }
  if (!answered) {
    run_loop.Run();
  }
  return result;
}

// Both data streams an HTTP entry has: the response headers and the body. The
// simple backend has a third for sparse ranges, which nothing here writes.
int64_t EntrySize(disk_cache::Entry* entry) {
  return entry->GetDataSize(0) + entry->GetDataSize(1);
}

}  // namespace

base::expected<std::unique_ptr<disk_cache::Backend>, std::string>
OpenCacheBackend(const base::FilePath& directory) {
  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  disk_cache::BackendResult result;
  bool answered = false;

  disk_cache::BackendResult immediate = disk_cache::CreateCacheBackend(
      net::DISK_CACHE, net::CACHE_BACKEND_SIMPLE,
      /*file_operations=*/nullptr, directory,
      // 0 means "size yourself from the volume", which is what the engine
      // passes when the caller named no limit. Reading a cache does not need
      // to agree with the writer about the ceiling.
      /*max_bytes=*/0,
      // kNeverReset, emphatically. The other two settings delete the cache
      // when it looks wrong, and a caller who asked to list a cache would find
      // it emptied by the asking.
      disk_cache::ResetHandling::kNeverReset,
      /*net_log=*/nullptr,
      /*cache_encryption_delegate=*/nullptr,
      base::BindOnce(
          [](base::RunLoop* loop, disk_cache::BackendResult* out,
             bool* answered, disk_cache::BackendResult value) {
            *out = std::move(value);
            *answered = true;
            loop->Quit();
          },
          &run_loop, &result, &answered));

  if (immediate.net_error != net::ERR_IO_PENDING) {
    result = std::move(immediate);
  } else if (!answered) {
    run_loop.Run();
  }

  if (result.net_error != net::OK || !result.backend) {
    return base::unexpected(
        base::StrCat({"could not open the cache at ", directory.AsUTF8Unsafe(),
                      ": ", net::ErrorToShortString(result.net_error),
                      ". Within one process a directory has one backend; if "
                      "an engine is running with this one, ask it instead"}));
  }
  return std::move(result.backend);
}

base::expected<std::vector<CacheEntry>, std::string> ListCacheEntries(
    disk_cache::Backend* backend) {
  if (!backend) {
    return base::unexpected("there is no cache to list");
  }

  std::vector<CacheEntry> entries;
  std::unique_ptr<disk_cache::Backend::Iterator> iterator =
      backend->CreateIterator();
  while (true) {
    disk_cache::EntryResult result = WaitForEntry(base::BindOnce(
        [](disk_cache::Backend::Iterator* iterator,
           disk_cache::EntryResultCallback callback) {
          return iterator->OpenNextEntry(std::move(callback));
        },
        iterator.get()));
    // ERR_FAILED is how the iterator says "that was the last one", so it is
    // the exit and not an error to report.
    if (result.net_error() != net::OK) {
      break;
    }
    disk_cache::Entry* entry = result.ReleaseEntry();
    if (!entry) {
      break;
    }
    CacheEntry listed;
    listed.key = entry->GetKey();
    listed.url =
        std::string(net::HttpCache::GetResourceURLFromHttpCacheKey(listed.key));
    listed.last_used = entry->GetLastUsed();
    listed.size = EntrySize(entry);
    entries.push_back(std::move(listed));
    entry->Close();
  }
  return entries;
}

base::expected<CacheClearResult, std::string> ClearCacheEntries(
    disk_cache::Backend* backend,
    const CacheClearOptions& options) {
  if (!backend) {
    return base::unexpected("there is no cache to clear");
  }

  CacheClearResult result;
  const int64_t before = WaitForInt64(base::BindOnce(
      [](disk_cache::Backend* backend,
         net::Int64CompletionOnceCallback callback) {
        return backend->CalculateSizeOfAllEntries(std::move(callback));
      },
      backend));
  result.bytes_before = before < 0 ? 0 : before;

  const bool filtered = !options.urls.empty() ||
                        options.unused_since.has_value() ||
                        options.max_bytes.has_value();

  if (!filtered) {
    // The whole thing, in one operation the backend can do far better than an
    // enumeration: the simple backend renames the directory aside and deletes
    // it in the background rather than unlinking entry by entry.
    // The cast is not cosmetic: these return net::Error, and the waiter takes
    // the int that every other completion in //net is spelled with.
    const int error = WaitForCompletion(base::BindOnce(
        [](disk_cache::Backend* backend,
           net::CompletionOnceCallback callback) -> int {
          return backend->DoomAllEntries(std::move(callback));
        },
        backend));
    if (error != net::OK) {
      return base::unexpected(base::StrCat({"could not clear the cache: ",
                                            net::ErrorToShortString(error)}));
    }
    result.removed = -1;  // Unknown, and not worth an enumeration to find out.
    result.bytes_after = 0;
    return result;
  }

  auto listed = ListCacheEntries(backend);
  if (!listed.has_value()) {
    return base::unexpected(listed.error());
  }

  const std::set<std::string> wanted(options.urls.begin(), options.urls.end());
  std::vector<const CacheEntry*> doomed;
  std::vector<const CacheEntry*> survivors;
  for (const CacheEntry& entry : *listed) {
    const bool by_url = !wanted.empty() && wanted.count(entry.url) > 0;
    const bool by_age = options.unused_since.has_value() &&
                        entry.last_used < *options.unused_since;
    if (by_url || by_age) {
      doomed.push_back(&entry);
    } else {
      survivors.push_back(&entry);
    }
  }

  // The size ceiling applies to what the filters above did not already take,
  // and it takes the least recently used first -- the same order the backend's
  // own eviction would, so a caller who sets `maxBytes` here and one who sets
  // `cacheMaxBytes` at startup get the same cache.
  if (options.max_bytes.has_value()) {
    int64_t remaining = 0;
    for (const CacheEntry* entry : survivors) {
      remaining += entry->size;
    }
    if (remaining > *options.max_bytes) {
      std::sort(survivors.begin(), survivors.end(),
                [](const CacheEntry* a, const CacheEntry* b) {
                  return a->last_used < b->last_used;
                });
      for (const CacheEntry* entry : survivors) {
        if (remaining <= *options.max_bytes) {
          break;
        }
        remaining -= entry->size;
        doomed.push_back(entry);
      }
    }
  }

  for (const CacheEntry* entry : doomed) {
    // By key, not by URL. GetResourceURLFromHttpCacheKey drops the partition
    // prefix and has no inverse, so a URL cannot be turned back into something
    // the backend would recognise -- which is why ListCacheEntries kept the
    // key it was already holding.
    const int error = WaitForCompletion(base::BindOnce(
        [](disk_cache::Backend* backend, const std::string& key,
           net::CompletionOnceCallback callback) -> int {
          return backend->DoomEntry(key, net::LOWEST, std::move(callback));
        },
        backend, entry->key));
    if (error == net::OK) {
      ++result.removed;
    }
  }

  const int64_t after = WaitForInt64(base::BindOnce(
      [](disk_cache::Backend* backend,
         net::Int64CompletionOnceCallback callback) {
        return backend->CalculateSizeOfAllEntries(std::move(callback));
      },
      backend));
  result.bytes_after = after < 0 ? 0 : after;
  return result;
}

}  // namespace shot
