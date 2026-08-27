// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_CAPTURE_CONTEXT_H_
#define SHOT_SHOT_CAPTURE_CONTEXT_H_

#include <map>
#include <string>

#include "base/functional/callback.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/http/http_request_headers.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace shot {

// What one capture turned out to cost, and where the bytes came from.
//
// Every number here is already known somewhere inside //net -- was_cached is
// read off the URLRequest in shot_fetch.cc and handed to blink, the status code
// reaches BuildHttpResponse, the timings bound sections of Capture(). None of
// it reached the caller, so "why did this screenshot take 350ms" could only be
// answered by rebuilding the engine with logging in it. It is collected here
// instead and returned with the image.
struct CaptureStats {
  // Every resource the document asked for, including itself. A file: URL
  // counts: it can fail, and a caller looking at `failed` wants to know.
  int requests = 0;
  // Answered out of the HTTP cache, which is URLRequest::was_cached() and
  // means the body came from disk. It does not mean no network was touched: a
  // stale-but-revalidatable entry costs a conditional request and a 304, and
  // that counts here too. What the cache saved in that case is the download,
  // not the round trip -- which is why `timing.fetch` can be tens of
  // milliseconds with this set.
  int from_cache = 0;
  int failed = 0;
  // Decoded body bytes, summed. Not the transfer size: the fetch layer buffers
  // after content-encoding, so a brotli response counts what blink was given
  // rather than what crossed the wire.
  int64_t bytes = 0;

  // The top-level document's own outcome. 0 for a file: URL, which has no
  // status to report.
  int http_status = 0;
  // After redirects, which is what relative URLs in the document resolved
  // against. Worth returning because a caller who asked for one URL and got a
  // picture of another has no other way to find out.
  std::string final_url;

  // Fetching the top-level document. This is the one that surprises people:
  // for a cold https:// URL it is DNS, TCP, TLS and the round trip, and it
  // dwarfs everything below.
  base::TimeDelta fetch;
  // Parse, subresource loading, style, layout, prepaint, paint.
  base::TimeDelta render;
  // Page/frame creation and synchronous document installation.
  base::TimeDelta setup;
  // Waiting for parsing, load completion and subresources.
  base::TimeDelta wait;
  // Capture-rectangle resolution plus style/layout/lifecycle advancement.
  base::TimeDelta lifecycle;
  // Extracting Blink's paint record after the lifecycle is clean.
  base::TimeDelta paint;
  // Allocating or reusing the raster surface and replaying the paint record.
  base::TimeDelta raster;
  base::TimeDelta encode;
  // Wall clock across the whole call, so the phases above can be checked
  // against it rather than assumed to be exhaustive.
  base::TimeDelta total;
};

// The parts of a request that no single call site can pass down.
//
// A screenshot is one document plus everything it references, and the
// references are discovered by blink's ResourceFetcher, which builds a
// ShotURLLoader for each. Nothing hands that loader the ScreenshotRequest --
// it is several layers below anything that has seen one -- so a per-request
// decision like "reload, do not read the cache" has nowhere to travel except
// alongside the capture itself. That is what this is.
//
// Scoped rather than global because it must not outlive the capture: a stale
// load flag would silently apply to the next document, and a stale header
// would send one caller's cookie with another caller's request.
class CaptureContext {
 public:
  CaptureContext();

  CaptureContext(const CaptureContext&) = delete;
  CaptureContext& operator=(const CaptureContext&) = delete;

  ~CaptureContext();

  // The capture in progress on this thread, or null between captures. Null is
  // an ordinary answer and every caller handles it: blink can fetch during
  // teardown, and the daemon's prewarm render happens before anyone asked for
  // statistics.
  static CaptureContext* Current();

  // net::LOAD_* for every request this capture makes. See CacheModeToLoadFlags.
  void set_load_flags(int flags) { load_flags_ = flags; }
  int load_flags() const { return load_flags_; }

  // Headers the caller supplied, and the origin they may be sent to.
  //
  // Same-origin only, and that is the whole rule. A caller passing
  // `Authorization` or `Cookie` means it for the site being photographed; a
  // document that pulls a script from a CDN must not have the credential
  // forwarded there, and there is no header a browser would forward
  // cross-origin that blink is not already setting for itself.
  void SetExtraHeaders(net::HttpRequestHeaders headers,
                       const url::Origin& origin);
  // The headers to add to a request for `url`, which is empty unless `url` is
  // same-origin with the document.
  net::HttpRequestHeaders HeadersFor(const GURL& url) const;

  // Called once per resource, from wherever it was resolved: the network
  // fetch, or the file: reader that never touches //net.
  void RecordResource(bool from_cache, bool failed, int64_t bytes);

  // Wakes ShotRenderer's load wait when a resource or frame event makes
  // progress. The 10 ms fallback remains as a correctness watchdog for Blink
  // state changes that have no embedder callback.
  void SetProgressCallback(base::RepeatingClosure callback);
  void NotifyProgress();

  CaptureStats& stats() { return stats_; }
  const CaptureStats& stats() const { return stats_; }

 private:
  CaptureStats stats_;
  base::RepeatingClosure progress_callback_;
  int load_flags_ = 0;
  net::HttpRequestHeaders extra_headers_;
  url::Origin headers_origin_;
  bool has_extra_headers_ = false;
};

// CaptureStats as the JSON object both entry points return.
//
// Here rather than beside either caller because there are two -- the resident
// worker writes it into its response header, the shared library serialises it
// for the addon -- and this is the published shape of the field: shotium's
// CaptureStats in types.ts declares the same names. Two spellings of it would
// be one spelling and one bug.
base::DictValue StatsToValue(const CaptureStats& stats);

// The four cache modes, spelled the way fetch() spells them.
//
// Borrowed rather than invented: a caller who knows what `only-if-cached` does
// in a browser knows what it does here, and a caller who does not has
// somewhere to go and read about it. Anything else this tree made up would
// have to be explained from scratch and would still mean one of these four.
//
// Returns false for a string that is not one of them.
bool CacheModeToLoadFlags(const std::string& mode, int* out_flags);

}  // namespace shot

#endif  // SHOT_SHOT_CAPTURE_CONTEXT_H_
