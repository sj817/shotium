// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_fetch.h"

#include <cstdlib>
#include <deque>
#include <map>
#include <set>
#include <string>
#include <utility>

#include "base/auto_reset.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
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

// How many bytes of response body may be held at once.
//
// A body exists in this process from its first byte until blink has consumed
// it, so what is arriving together is what is resident together. Over the
// network //net has a per-host connection limit and that keeps the number
// small by accident: six sockets, six bodies. Against a warm disk cache
// nothing does -- every entry answers in the same task -- and a page with
// ninety subresources allocated all ninety bodies at once, which on a page of
// photographs was seventy megabytes that the same page fetched over the
// network never came near.
//
// What waits is the read, not the request. Holding requests back at the start
// would have punished the ninety small ones for the six large ones: measured
// on a page whose sixty font subsets all fail, a cap of six starts cost 20% of
// the wall clock, because sixty connections that would have failed together
// failed ten at a time instead. So every request is started at once, and a
// response whose Content-Length would not fit simply does not begin reading
// until there is room for it. Below the budget nothing waits at all.
//
// A response with no Content-Length cannot say at the start what it will
// cost: chunked encoding says how long a body is by ending it. One of those
// starts reading whenever the budget is not already full, and is stopped in
// the middle if it fills it -- //net reads nothing it is not asked for, so a
// read that stops asking stops costing -- and resumed when there is room
// again.
//
// Which leaves the question of who may go on when there is no room, because
// somebody must or nothing ever finishes. A body a loader is holding needs
// nothing from here: it drains into blink on its own, and everyone can afford
// to wait for it. So the answer is only needed when every byte in flight is
// still arriving, and then it is the oldest of those reads. It finishes, its
// room comes back, and the next one along becomes the oldest.
//
// 24 MB: above any single resource these pages carry, so no request ever waits
// alone, and a sixth of what the unbounded version reached.
// SHOT_FETCH_BUDGET_MB overrides it; 0 means no limit.
constexpr size_t kDefaultBudgetMb = 24;

size_t ReadBudgetBytes() {
  static const size_t budget = [] {
    const char* value = std::getenv("SHOT_FETCH_BUDGET_MB");
    int parsed = 0;
    if (value && *value && base::StringToInt(value, &parsed) && parsed >= 0) {
      return static_cast<size_t>(parsed) << 20;
    }
    return kDefaultBudgetMb << 20;
  }();
  return budget;
}

// How many requests to one host may be running at once.
//
// This is not a policy of ours; it is the number //net already enforces when
// it has to open a socket, and the reason to state it here is that a cache hit
// does not open one. Over the network six requests to a host means six of that
// host's entries being read at a time. Against a warm disk cache the same page
// opened all thirty-four of its images in one task, and //net holds a buffer
// per open entry -- which is why the cached run of a page of photographs
// peaked fifty megabytes above the same page fetched over the network, and why
// bounding our own reads (below) did not help: the memory was allocated before
// we asked for a byte.
//
// SHOT_FETCH_CONCURRENCY overrides it; 0 means no limit.
constexpr int kDefaultMaxPerHost = 6;

int MaxPerHost() {
  static const int limit = [] {
    const char* value = std::getenv("SHOT_FETCH_CONCURRENCY");
    int parsed = 0;
    if (value && *value && base::StringToInt(value, &parsed) && parsed >= 0) {
      return parsed;
    }
    return kDefaultMaxPerHost;
  }();
  return limit;
}

// Per host: how many are running, and the ones waiting to start, oldest first.
std::map<std::string, int>& InFlightPerHost() {
  static base::NoDestructor<std::map<std::string, int>> counts;
  return *counts;
}
std::map<std::string, std::deque<base::OnceClosure>>& QueuedPerHost() {
  static base::NoDestructor<std::map<std::string, std::deque<base::OnceClosure>>>
      queues;
  return *queues;
}

// What the response bodies in memory hold between them. Main-thread only:
// every URLRequest here lives on it, and so does every loader holding one of
// their bodies afterwards.
size_t g_bytes_in_flight = 0;

// The part of that which is still arriving, as against the part some loader is
// holding until blink has consumed it.
//
// The difference decides who has to wait. A body a loader is holding gives its
// room back on its own -- the data pipe drains as blink reads it, and that
// needs nothing from any request -- so everything else can afford to wait for
// it. A body still being read gives its room back only by being read to the
// end, so when that is all there is, one of them has to be let through.
size_t g_bytes_reading = 0;

// The most `g_bytes_in_flight` has been. There is no end of capture to report
// it at -- fetches outlive every object here -- so it is logged as it grows,
// which is also where the interesting moment is.
size_t g_peak_bytes_in_flight = 0;

bool FetchProfileEnabled() {
  static const bool enabled = [] {
    const char* value = std::getenv("SHOT_PROFILE");
    int parsed = 0;
    return value && *value && base::StringToInt(value, &parsed) && parsed != 0;
  }();
  return enabled;
}

void NotePeakBytes() {
  if (g_bytes_in_flight <= g_peak_bytes_in_flight) {
    return;
  }
  g_peak_bytes_in_flight = g_bytes_in_flight;
  if (FetchProfileEnabled()) {
    LOG(INFO) << "shot: fetch peak " << (g_bytes_in_flight >> 10) << " KB in "
              << "flight (" << (g_bytes_reading >> 10) << " KB arriving, "
              << ((g_bytes_in_flight - g_bytes_reading) >> 10)
              << " KB delivered)";
  }
}

// The bodies currently being read, in the order they started. The lowest is
// the oldest, which is the one that goes on when nothing else can give room
// back -- see the note on the budget above.
std::set<uint64_t>& ActiveReads() {
  static base::NoDestructor<std::set<uint64_t>> reads;
  return *reads;
}
uint64_t g_next_read_seq = 1;

// A read that has not started: what it said it would take, and how to start
// it when there is room for that.
struct WaitingRead {
  size_t wanted = 0;
  base::OnceClosure start;
};

// Those, oldest first, and asked in that order: each takes its room as it
// starts and the next may no longer fit behind it, and a large one at the
// front is what keeps the small ones behind it from starving it.
std::deque<WaitingRead>& WaitingReads() {
  static base::NoDestructor<std::deque<WaitingRead>> queue;
  return *queue;
}

// Reads that started and then stopped mid-body, because what arrived since
// put the process over budget. Held as weak pointers rather than closures so
// that each one is asked again whether it may go, instead of being handed a
// decision made when it was queued.
std::deque<base::WeakPtr<ShotFetch>>& PausedReads() {
  static base::NoDestructor<std::deque<base::WeakPtr<ShotFetch>>> queue;
  return *queue;
}

// Room for `wanted` more bytes, or a reason to say yes without it.
//
// Over budget this read waits, and waiting is only safe while there is
// something to wait for. Everything in flight gives its room back without
// needing a byte from here: a body a loader is holding as blink consumes it,
// a body still arriving by arriving. With nothing in flight at all there is
// nothing to wait for, and a read that does not fit even then is a single
// resource larger than the whole budget -- refusing that would be refusing it
// forever.
bool MayReadNow(size_t wanted) {
  return ReadBudgetBytes() == 0 || g_bytes_in_flight == 0 ||
         g_bytes_in_flight + wanted <= ReadBudgetBytes();
}

// Set while reads are being let through, so that one of them finishing --
// which gives its room back, which lets more through -- does not start a
// second pass inside the first. The outer pass asks again after every read it
// resumes, so anything the inner one would have found is found there.
bool g_resuming = false;

// Lets through what the room just given back allows: first the reads that
// stopped mid-body, because they are already holding their bytes and
// finishing one is what gives room back, then the reads that have not
// started.
void ResumeReads() {
  if (g_resuming) {
    return;
  }
  base::AutoReset<bool> resuming(&g_resuming, true);

  for (size_t i = 0; i < PausedReads().size();) {
    base::WeakPtr<ShotFetch> paused = PausedReads()[i];
    if (!paused) {
      PausedReads().erase(PausedReads().begin() + i);
      continue;
    }
    if (!paused->MayContinueReading()) {
      ++i;
      continue;
    }
    PausedReads().erase(PausedReads().begin() + i);
    // May finish the request, and so may run this function again -- which the
    // flag above turns into a no-op, leaving this loop to notice the room.
    paused->Resume();
  }

  while (!WaitingReads().empty() &&
         MayReadNow(WaitingReads().front().wanted)) {
    // Asked one at a time rather than admitted in a batch: the one that starts
    // here takes its room as it starts, and the next is asked again with that
    // taken. A weak-pointer bind whose target is gone reads nothing and takes
    // no room, and the next one along is then simply asked on its own terms.
    WaitingRead waiting = std::move(WaitingReads().front());
    WaitingReads().pop_front();
    std::move(waiting.start).Run();
  }
}

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

FetchCharge::FetchCharge(FetchCharge&& other) : bytes_(other.bytes_) {
  other.bytes_ = 0;
}

FetchCharge& FetchCharge::operator=(FetchCharge&& other) {
  if (this != &other) {
    Release();
    bytes_ = other.bytes_;
    other.bytes_ = 0;
  }
  return *this;
}

FetchCharge::~FetchCharge() {
  Release();
}

void FetchCharge::GrowTo(size_t total) {
  if (total <= bytes_) {
    return;
  }
  g_bytes_in_flight += total - bytes_;
  bytes_ = total;
}

void FetchCharge::Release() {
  if (bytes_ == 0) {
    return;
  }
  g_bytes_in_flight -= bytes_;
  bytes_ = 0;
  ResumeReads();
}

ShotFetch::ShotFetch() = default;

ShotFetch::~ShotFetch() {
  // First, because giving the budget back below lets other reads run, and one
  // of the places they are reached from is a list of weak pointers that may
  // still hold one to this. A weak pointer stays valid until the factory is
  // destroyed, which is after this body, so a read abandoned while paused
  // would otherwise be resumed on an object halfway through going away.
  weak_factory_.InvalidateWeakPtrs();
  // Abandoned before it finished -- a loader destroyed to cancel the load, or
  // the whole capture torn down. Whoever is queued behind it should not wait
  // for a request that is no longer running.
  ReleaseBudget();
  ReleaseHostSlot();
}

void ShotFetch::Start(const GURL& url,
                      const net::HttpRequestHeaders& extra_headers,
                      const url::Origin& initiator,
                      DoneCallback done) {
  done_ = std::move(done);
  result_.final_url = url;

  host_ = url.host();
  int& running = InFlightPerHost()[host_];
  if (MaxPerHost() > 0 && running >= MaxPerHost()) {
    // Queued, not refused. The callback still cannot run before this returns,
    // which is what Start() promises; it merely runs later.
    QueuedPerHost()[host_].push_back(
        base::BindOnce(&ShotFetch::StartNow, weak_factory_.GetWeakPtr(), url,
                       extra_headers, initiator));
    return;
  }
  ++running;
  holds_slot_ = true;
  StartNow(url, extra_headers, initiator);
}

void ShotFetch::StartNow(const GURL& url,
                         const net::HttpRequestHeaders& extra_headers,
                         const url::Origin& initiator) {
  if (!holds_slot_) {
    // Reached from the queue, where the slot was not taken yet.
    ++InFlightPerHost()[host_];
    holds_slot_ = true;
  }
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

// Gives back this request's place among its host's and starts the next one
// waiting for it. Called once, whether the request succeeded, failed or was
// abandoned.
void ShotFetch::ReleaseHostSlot() {
  if (!holds_slot_) {
    return;
  }
  holds_slot_ = false;
  int& running = InFlightPerHost()[host_];
  --running;
  auto queued = QueuedPerHost().find(host_);
  while (queued != QueuedPerHost().end() && !queued->second.empty() &&
         (MaxPerHost() <= 0 || running < MaxPerHost())) {
    // A weak-pointer bind whose target is gone starts nothing and takes no
    // slot, so keep going until one of them actually starts.
    const int before = running;
    base::OnceClosure start = std::move(queued->second.front());
    queued->second.pop_front();
    std::move(start).Run();
    if (running != before) {
      break;
    }
  }
}

// Stops this body counting as one that is being read, and gives back whatever
// of the budget it is still holding.
//
// The two are not the same event and only one of them is here. A body that
// was delivered has stopped reading but has not stopped costing: its claim
// went with it into the result and comes back when the bytes do. What this
// releases is the claim of a body that never got that far -- one that failed,
// or one whose request was abandoned.
void ShotFetch::ReleaseBudget() {
  StopReading();
  charge_.Release();
  // Even with nothing to give back, this request is no longer among those the
  // oldest is measured against, and the next one along may now be it.
  ResumeReads();
}

// Takes this body out of the reads the budget arbitrates between, without
// touching what it is holding. Called once, whether the body was delivered,
// failed or abandoned.
void ShotFetch::StopReading() {
  if (read_seq_ != 0) {
    ActiveReads().erase(read_seq_);
    read_seq_ = 0;
  }
  // Whatever this body holds it no longer holds as one that is arriving --
  // either it has been handed on, in which case the claim went with it, or it
  // is being given back on the next line of the caller.
  g_bytes_reading -= reading_bytes_;
  reading_bytes_ = 0;
  paused_ = false;
}

bool ShotFetch::MayContinueReading() const {
  if (ReadBudgetBytes() == 0 || g_bytes_in_flight <= ReadBudgetBytes()) {
    return true;
  }
  if (g_bytes_in_flight > g_bytes_reading) {
    // Some of what is over the budget is a body a loader is holding, and that
    // comes back as blink consumes it without anything being read for it. So
    // everything here waits for that rather than adding to it.
    return false;
  }
  // Everything in flight is still arriving, so one of these reads has to be
  // the one that finishes: the oldest goes on, and when it is done the next
  // one along is the oldest.
  return !ActiveReads().empty() && *ActiveReads().begin() == read_seq_;
}

void ShotFetch::Resume() {
  paused_ = false;
  ReadMore();
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
  // multi-megabyte image is not grown a buffer at a time -- and wait, if that
  // reservation would not fit alongside the bodies already in memory. A
  // response with no Content-Length reserves nothing and asks for nothing, so
  // it waits only while the budget is full; what it turns out to cost is
  // counted as it arrives, and stops it in the middle if it goes too far.
  const int64_t declared = request->GetExpectedContentSize();
  const size_t expected =
      declared > 0 && static_cast<size_t>(declared) <= kMaximumBodyBytes
          ? static_cast<size_t>(declared)
          : 0;
  if (!MayReadNow(expected)) {
    WaitingReads().push_back(
        {expected, base::BindOnce(&ShotFetch::BeginReading,
                                  weak_factory_.GetWeakPtr(), expected)});
    return;
  }
  BeginReading(expected);
}

void ShotFetch::BeginReading(size_t expected) {
  read_seq_ = g_next_read_seq++;
  ActiveReads().insert(read_seq_);
  if (expected != 0) {
    result_.body.reserve(expected);
    HoldBytes(expected);
  }
  ReadMore();
}

void ShotFetch::HoldBytes(size_t total) {
  charge_.GrowTo(total);
  if (total > reading_bytes_) {
    g_bytes_reading += total - reading_bytes_;
    reading_bytes_ = total;
  }
  NotePeakBytes();
}

void ShotFetch::OnReadCompleted(net::URLRequest* request, int bytes_read) {
  if (Consume(bytes_read)) {
    ReadMore();
  }
}

void ShotFetch::ReadMore() {
  while (true) {
    if (!MayContinueReading()) {
      // Over budget with older bodies still reading. Stopping here stops the
      // cost: //net hands over nothing that was not asked for, so a socket
      // left unread holds its bytes in the kernel rather than in this
      // process. ResumeReads() picks this up when there is room.
      if (!paused_) {
        paused_ = true;
        PausedReads().push_back(weak_factory_.GetWeakPtr());
      }
      return;
    }
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
  // A body that outgrew its Content-Length, or arrived without one, is
  // counted as it comes. What that costs is decided at the top of ReadMore,
  // which is where a body that has taken more than its share stops.
  HoldBytes(result_.body.size());
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
  // The claim on the budget goes with the body. Whoever takes the result
  // holds the bytes and holds what they cost until it drops them; releasing
  // here instead would count a body out of the budget while it was still in
  // the process, which is the whole thing the budget is trying to bound.
  result_.charge = std::move(charge_);
  StopReading();
  ReleaseHostSlot();
  if (done_) {
    std::move(done_).Run(std::move(result_));
  }
  // After the callback, because until it has run nobody else can know what
  // the body cost or whether it is still being held.
  ResumeReads();
}

}  // namespace shot
