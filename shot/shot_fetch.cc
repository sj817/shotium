// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_fetch.h"

#include <utility>

#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_view_util.h"
#include "base/task/single_thread_task_runner.h"
#include "net/base/isolation_info.h"
#include "net/base/load_flags.h"
#include "net/base/network_handle.h"
#include "net/base/net_errors.h"
#include "net/cookies/site_for_cookies.h"
#include "net/http/http_request_headers.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/url_request/redirect_info.h"
#include "net/url_request/url_request_context.h"
#include "shot/shot_capture_context.h"
#include "shot/shot_network.h"

namespace shot {
namespace {

// One read at a time, 64 KiB each. Large enough that a typical stylesheet or
// font arrives in one or two passes, small enough that a request that is
// abandoned mid-body is not holding a megabyte.
constexpr int kReadBufferSize = 64 * 1024;

// A ceiling, not a policy about what is reasonable to fetch. Without one, a
// response with no Content-Length and a server that never stops is an
// out-of-memory rather than an error message.
constexpr size_t kMaximumBodyBytes = 64u * 1024u * 1024u;

// net::URLRequest enforces its own limit of 20 as well; this one is here so
// that the failure names redirects rather than arriving as a bare ERR_ code.
constexpr int kMaximumRedirects = 20;

constexpr net::NetworkTrafficAnnotationTag kTrafficAnnotation =
    net::DefineNetworkTrafficAnnotation("shotium_page_resource", R"(
        semantics {
          sender: "shotium screenshot worker"
          description:
            "Fetches a page and the subresources it references -- stylesheets, "
            "images, fonts -- so that they can be laid out and rasterised into "
            "a screenshot. The URL is the one the caller of the library asked "
            "to photograph, plus whatever that document links to."
          trigger:
            "A screenshot request naming an http or https URL, or a document "
            "that references one."
          data:
            "The request URL, the User-Agent, and cookies previously set by "
            "the same origin within this worker's lifetime. Nothing about the "
            "host machine is added."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: YES
          cookies_store: "An in-memory store that is discarded when the worker "
            "process exits; nothing is written to disk."
          setting:
            "This is a standalone library, not a browser feature; there is no "
            "user-facing setting. The caller controls it by controlling which "
            "URLs it asks for."
          policy_exception_justification:
            "Not a Chrome feature -- shotium is a screenshot library and has "
            "no enterprise policy surface."
        })");

}  // namespace

ShotFetch::ShotFetch() = default;

ShotFetch::~ShotFetch() = default;

void ShotFetch::Start(const GURL& url,
                      const net::HttpRequestHeaders& extra_headers,
                      const url::Origin& initiator,
                      DoneCallback done) {
  done_ = std::move(done);
  result_.final_url = url;

  net::URLRequestContext* context = ShotNetwork::Get();
  if (!context) {
    // Posting rather than calling: Start() promises the callback does not run
    // before it returns, and a caller that has not finished wiring itself up
    // would otherwise be re-entered here.
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&ShotFetch::Finish, weak_factory_.GetWeakPtr(),
                                  net::ERR_INTERNET_DISCONNECTED));
    return;
  }

  // The five-argument form, because the four-argument one is compiled out on
  // desktop Linux and Windows -- those are the platforms whose annotations are
  // audited, so an unannotated request is not offered there at all. The network
  // handle names a specific interface to bind to, which is an Android
  // multi-network feature; kInvalidNetworkHandle means "the default route".
  request_ = context->CreateRequest(url, net::RequestPriority::MEDIUM, this,
                                    kTrafficAnnotation,
                                    net::handles::kInvalidNetworkHandle);
  request_->set_method("GET");

  // The caller's headers, then the capture's. Both are "extra" in the same
  // sense -- neither is something //net would have sent for itself -- but they
  // arrive from different places: blink fills in Accept and Referer per
  // resource, while the capture carries what the host program asked for. The
  // capture's win a collision, because a host that set Authorization meant it.
  net::HttpRequestHeaders headers = extra_headers;
  if (CaptureContext* capture = CaptureContext::Current()) {
    headers.MergeFrom(capture->HeadersFor(url));
    // Applied here rather than in the context because this is the only place
    // that has a URLRequest to apply it to, and because a request built while
    // no capture is in progress must keep //net's defaults rather than the
    // last capture's.
    request_->SetLoadFlags(capture->load_flags());
  }
  request_->SetExtraRequestHeaders(headers);

  // The isolation info decides which cache and cookie partition this request
  // lands in. Attributing every subresource to the document's origin is what a
  // browser does for a top-level document and everything under it, and it is
  // what makes two runs over the same page hit the same cache entries.
  request_->set_isolation_info(net::IsolationInfo::Create(
      net::IsolationInfo::RequestType::kOther, initiator, initiator,
      net::SiteForCookies::FromOrigin(initiator)));
  request_->set_initiator(initiator);

  buffer_ = base::MakeRefCounted<net::IOBufferWithSize>(kReadBufferSize);
  request_->Start();
}

void ShotFetch::OnReceivedRedirect(net::URLRequest* request,
                                   const net::RedirectInfo& redirect_info,
                                   bool* defer_redirect) {
  *defer_redirect = false;
  if (++redirects_ > kMaximumRedirects) {
    request_->Cancel();
    Finish(net::ERR_TOO_MANY_REDIRECTS);
    return;
  }
  VLOG(1) << "shot: redirect " << redirects_ << " -> "
          << redirect_info.new_url.spec();
}

void ShotFetch::OnResponseStarted(net::URLRequest* request, int net_error) {
  if (net_error != net::OK) {
    Finish(net_error);
    return;
  }

  result_.final_url = request->url();
  result_.headers = request->response_headers();
  result_.http_status = request->GetResponseCode();
  result_.was_cached = request->was_cached();
  request->GetMimeType(&result_.mime_type);
  request->GetCharset(&result_.charset);

  // Reserve from Content-Length when the server gave a believable one, so a
  // multi-megabyte image is not grown a buffer at a time.
  const int64_t expected = request->GetExpectedContentSize();
  if (expected > 0 && static_cast<size_t>(expected) <= kMaximumBodyBytes) {
    result_.body.reserve(static_cast<size_t>(expected));
  }

  ReadMore();
}

void ShotFetch::OnReadCompleted(net::URLRequest* request, int bytes_read) {
  if (Consume(bytes_read)) {
    ReadMore();
  }
}

void ShotFetch::ReadMore() {
  while (true) {
    const int rv = request_->Read(buffer_.get(), buffer_->size());
    if (rv == net::ERR_IO_PENDING) {
      // OnReadCompleted resumes the loop.
      return;
    }
    if (!Consume(rv)) {
      return;
    }
  }
}

bool ShotFetch::Consume(int bytes_read) {
  if (bytes_read < 0) {
    Finish(bytes_read);
    return false;
  }
  if (bytes_read == 0) {
    Finish(net::OK);
    return false;
  }
  if (result_.body.size() + static_cast<size_t>(bytes_read) >
      kMaximumBodyBytes) {
    request_->Cancel();
    Finish(net::ERR_FILE_TOO_BIG);
    return false;
  }
  result_.body.append(
      base::as_string_view(buffer_->first(static_cast<size_t>(bytes_read))));
  return true;
}

void ShotFetch::Finish(int net_error) {
  result_.net_error = net_error;
  // Counted here rather than at each call site because this is the one place
  // every http(s) resource passes through exactly once, whether it succeeded,
  // failed, redirected or came out of the cache. A count kept by the callers
  // would be two counts that agree until one of them grows a new early return.
  if (CaptureContext* capture = CaptureContext::Current()) {
    capture->RecordResource(result_.was_cached, net_error != net::OK,
                            static_cast<int64_t>(result_.body.size()));
    capture->NotifyProgress();
  }
  // The request is done with; dropping it here means a callback that renders
  // synchronously is not doing so with a live URLRequest underneath it.
  request_.reset();
  buffer_.reset();
  if (done_) {
    std::move(done_).Run(std::move(result_));
  }
}

}  // namespace shot
