// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_FETCH_H_
#define SHOT_SHOT_FETCH_H_

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

 private:
  // net::URLRequest::Delegate:
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

  base::WeakPtrFactory<ShotFetch> weak_factory_{this};
};

}  // namespace shot

#endif  // SHOT_SHOT_FETCH_H_
