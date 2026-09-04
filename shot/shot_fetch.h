// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_FETCH_H_
#define SHOT_SHOT_FETCH_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace net {
class HttpRequestHeaders;
}

namespace shot {

// A claim on the bytes a response body may hold, kept for as long as the body
// it was taken for is in memory.
//
// The claim outlives the request on purpose. A fetch that has finished does
// not stop costing anything: its body is moved to the loader, which holds it
// until the data pipe has carried it into blink. Giving the budget back when
// the request ended would have counted a body out while it was still in the
// process -- and on a page whose subresources all answer from the disk cache,
// which is the case the budget was written for, that is every body at once.
// So the claim travels with the bytes, in FetchResult, and is dropped where
// they are.
//
// Moving one moves the claim; the moved-from object holds nothing. Main-thread
// only, like everything else the budget touches.
class FetchCharge {
 public:
  FetchCharge() = default;
  FetchCharge(FetchCharge&& other);
  FetchCharge& operator=(FetchCharge&& other);
  FetchCharge(const FetchCharge&) = delete;
  FetchCharge& operator=(const FetchCharge&) = delete;
  ~FetchCharge();

  size_t bytes() const { return bytes_; }

  // Raises the claim to `total`. Never lowers it: a claim that shrank would
  // let a read that is still holding the memory hand it out to somebody else.
  // Taking the room is not the same as waiting for it -- waiting is the
  // caller's decision, made before it gets this far.
  void GrowTo(size_t total);

  // Gives the claim back and lets the reads waiting on it go. Idempotent, and
  // what the destructor does.
  void Release();

 private:
  size_t bytes_ = 0;
};

// What one http(s) GET produced.
struct FetchResult {
  // net::OK, or the reason it failed. Everything else is only meaningful when
  // this is net::OK.
  int net_error = 0;

  int http_status = 0;
  std::string mime_type;
  std::string charset;

  // Where the response actually came from, after any redirects. Relative URLs
  // inside the body resolve against this, not against the requested URL.
  GURL final_url;

  scoped_refptr<const net::HttpResponseHeaders> headers;
  std::string body;
  bool was_cached = false;

  // What `body` costs against the read budget. Held by whoever holds the
  // body: destroy it, or move it somewhere that outlives the bytes, but do
  // not drop it while the bytes are still around.
  FetchCharge charge;
};

// One http(s) GET, buffered whole.
//
// Buffered rather than streamed because every consumer here wants the complete
// bytes: a subresource becomes a SharedBuffer, and the top-level document is
// handed to LocalFrame::ForceSynchronousDocumentInstall, which takes all of it
// at once. Streaming would add a state machine for no gain -- with a ceiling
// on the body size instead of a promise not to run out of memory.
//
// Lives on the thread that owns the URLRequestContext. Destroying it cancels
// the request, and the callback then does not run.
class ShotFetch : public net::URLRequest::Delegate {
 public:
  using DoneCallback = base::OnceCallback<void(FetchResult)>;

  ShotFetch();
  ShotFetch(const ShotFetch&) = delete;
  ShotFetch& operator=(const ShotFetch&) = delete;
  ~ShotFetch() override;

  // Starts the request. `done` runs exactly once, never before this returns.
  // `initiator` is the origin the request is attributed to: it decides the
  // cache and cookie partition, so a subresource has to pass the document's
  // origin rather than its own.
  void Start(const GURL& url,
             const net::HttpRequestHeaders& extra_headers,
             const url::Origin& initiator,
             DoneCallback done);

  // The read budget's two questions, public because the budget asks them from
  // outside this object: it holds the paused reads and decides which of them
  // goes next. Nothing else has any business calling either.
  //
  // Whether this body may ask //net for more bytes right now. False when what
  // is in flight is over budget and somebody else will bring it back down --
  // a loader draining a delivered body, or an older read finishing; the read
  // then parks itself until Resume() brings it back.
  bool MayContinueReading() const;
  void Resume();

 private:
  // net::URLRequest::Delegate:
  // Start(), once this request has a place among its host's. Start() calls it
  // directly when there is room and queues it when there is not.
  void StartNow(const GURL& url,
                const net::HttpRequestHeaders& extra_headers,
                const url::Origin& initiator);
  // Continues a redirect that waited for a place among the destination
  // host's requests. The queued closure is weak, so abandoning the fetch while
  // it waits cancels this as well as the URLRequest.
  void FollowDeferredRedirect();
  void ReleaseHostSlot();
  // Reads the body, once there is room for it. OnResponseStarted calls this
  // directly when there is and queues it when there is not.
  void BeginReading(size_t expected);
  // Raises what this body holds, and what it holds as a body still arriving.
  void HoldBytes(size_t total);
  void ReleaseBudget();
  void StopReading();

  void OnReceivedRedirect(net::URLRequest* request,
                          const net::RedirectInfo& redirect_info,
                          bool* defer_redirect) override;
  void OnResponseStarted(net::URLRequest* request, int net_error) override;
  void OnReadCompleted(net::URLRequest* request, int bytes_read) override;

  // Pulls from the request until it blocks, ends, or fails.
  void ReadMore();
  // Folds one Read() outcome into the body. Returns false once the request is
  // over, in which case Finish() has already run.
  bool Consume(int bytes_read);
  void Finish(int net_error);

  std::unique_ptr<net::URLRequest> request_;
  scoped_refptr<net::IOBufferWithSize> buffer_;
  DoneCallback done_;
  FetchResult result_;
  int redirects_ = 0;
  // What this body has taken from the read budget so far. Moved into the
  // result when the body is handed on, so that the claim follows the bytes.
  FetchCharge charge_;
  // The same number, counted again among the bodies that are still arriving
  // rather than waiting to be consumed. Kept here because the charge stops
  // being this body's when it moves into the result, and that is the moment
  // the bytes stop being ones anybody is waiting on //net for.
  size_t reading_bytes_ = 0;
  // Where this body comes in the order the bodies started reading, or 0 when
  // it is not reading. The oldest one is the one let through when no delivered
  // body is left to wait for.
  uint64_t read_seq_ = 0;
  // Whether this read is sitting in the paused list, so that it is put there
  // once rather than once per attempt.
  bool paused_ = false;
  // The host this request counts against, and whether it is counted. Kept
  // rather than re-read from the request, which redirects may have moved.
  std::string host_;
  bool holds_slot_ = false;

  base::WeakPtrFactory<ShotFetch> weak_factory_{this};
};

}  // namespace shot

#endif  // SHOT_SHOT_FETCH_H_
