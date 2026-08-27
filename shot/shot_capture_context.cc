// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_capture_context.h"

#include <utility>

#include "base/check.h"
#include "net/base/load_flags.h"

namespace shot {
namespace {

// The capture in progress, on the thread blink runs on.
//
// A plain pointer and not a TLS slot: everything that reads it -- ShotFetch,
// ShotURLLoader, the renderer -- is already required to be on the engine
// thread, because that is where the URLRequestContext and blink both live.
// Making it thread-local would suggest a second capture could be running
// somewhere, and there is no such thread.
CaptureContext* g_current = nullptr;

}  // namespace

CaptureContext::CaptureContext() {
  // Nested captures would mean one document's subresources counted against
  // another's, and the only way to get here twice is a bug in the caller
  // rather than a condition to recover from.
  CHECK(!g_current) << "a capture is already in progress on this thread";
  g_current = this;
}

CaptureContext::~CaptureContext() {
  g_current = nullptr;
}

// static
CaptureContext* CaptureContext::Current() {
  return g_current;
}

void CaptureContext::SetExtraHeaders(net::HttpRequestHeaders headers,
                                     const url::Origin& origin) {
  extra_headers_ = std::move(headers);
  headers_origin_ = origin;
  has_extra_headers_ = !extra_headers_.IsEmpty();
}

net::HttpRequestHeaders CaptureContext::HeadersFor(const GURL& url) const {
  if (!has_extra_headers_ ||
      !headers_origin_.IsSameOriginWith(url::Origin::Create(url))) {
    return net::HttpRequestHeaders();
  }
  return extra_headers_;
}

void CaptureContext::RecordResource(bool from_cache,
                                    bool failed,
                                    int64_t bytes) {
  ++stats_.requests;
  if (failed) {
    ++stats_.failed;
    return;
  }
  if (from_cache) {
    ++stats_.from_cache;
  }
  stats_.bytes += bytes;
}

base::DictValue StatsToValue(const CaptureStats& stats) {
  base::DictValue timing;
  timing.Set("fetch", stats.fetch.InMillisecondsF());
  timing.Set("render", stats.render.InMillisecondsF());
  timing.Set("encode", stats.encode.InMillisecondsF());
  timing.Set("total", stats.total.InMillisecondsF());

  base::DictValue dict;
  dict.Set("requests", stats.requests);
  dict.Set("fromCache", stats.from_cache);
  dict.Set("failed", stats.failed);
  // As a double: base::Value's int is 32 bits, and a page carrying a video
  // poster can exceed that. JSON has one number type anyway, and JS reads it
  // back into the same double it would have used for an int.
  dict.Set("bytes", static_cast<double>(stats.bytes));
  dict.Set("httpStatus", stats.http_status);
  dict.Set("finalUrl", stats.final_url);
  dict.Set("timing", std::move(timing));
  return dict;
}

bool CacheModeToLoadFlags(const std::string& mode, int* out_flags) {
  if (mode == "default") {
    *out_flags = net::LOAD_NORMAL;
    return true;
  }
  if (mode == "reload") {
    // Read nothing, write everything. This is what a browser's reload button
    // does: the response still populates the cache, so the *next* capture is
    // fast again.
    *out_flags = net::LOAD_BYPASS_CACHE;
    return true;
  }
  if (mode == "no-store") {
    // Neither read nor write. For a caller who does not want the page on this
    // machine's disk afterwards -- which a screenshot of an authenticated page
    // is a good reason to be.
    *out_flags = net::LOAD_DISABLE_CACHE;
    return true;
  }
  if (mode == "only-if-cached") {
    // The network is not allowed to be touched, and a miss is an error rather
    // than a fetch. SKIP_CACHE_VALIDATION with it because a stale-but-present
    // entry that would need revalidating is still an entry, and revalidating
    // it is exactly the round trip this mode exists to refuse.
    *out_flags = net::LOAD_ONLY_FROM_CACHE | net::LOAD_SKIP_CACHE_VALIDATION;
    return true;
  }
  return false;
}

}  // namespace shot
