// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_capture.h"

#include <utility>

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/functional/function_ref.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "shot/shot_capture_context.h"
#include "shot/shot_fetch.h"
#include "shot/shot_network.h"
#include "shot/shot_renderer.h"
#include "shot/shot_runtime.h"
#include "url/gurl.h"
#include "url/origin.h"
#include "url/url_constants.h"

namespace shot {
namespace {

// `file` is whatever the caller put in the request: a URL, or a path they
// expect to be treated as one. Resolving it here rather than in the JS layer
// keeps the two callers agreeing about what a bare path means.
base::expected<GURL, std::string> ResolveTarget(const std::string& file) {
  GURL url(file);
  if (url.is_valid() && url.has_scheme()) {
    return url;
  }

  base::FilePath path = base::FilePath::FromUTF8Unsafe(file);
  if (!path.IsAbsolute()) {
    base::FilePath current;
    if (base::GetCurrentDirectory(&current)) {
      path = current.Append(path);
    }
  }
  GURL from_path = net::FilePathToFileURL(path);
  if (!from_path.is_valid()) {
    return base::unexpected(base::StrCat({"cannot resolve ", file}));
  }
  return from_path;
}

bool IsNetworkScheme(const GURL& url) {
  return url.SchemeIs(url::kHttpScheme) || url.SchemeIs(url::kHttpsScheme);
}

// Reads the top-level document off the disk.
base::expected<RenderInput, std::string> ReadLocalDocument(const GURL& url) {
  base::FilePath path;
  if (!net::FileURLToFilePath(url, &path)) {
    return base::unexpected(base::StrCat({url.spec(), " is not a local file"}));
  }
  RenderInput input;
  input.url = url;
  if (!base::ReadFileToString(path, &input.body)) {
    if (CaptureContext* context = CaptureContext::Current()) {
      context->RecordResource(/*from_cache=*/false, /*failed=*/true, 0);
    }
    return base::unexpected(
        base::StrCat({"could not read ", path.AsUTF8Unsafe()}));
  }
  // Counted like any other resource. This is the top-level document rather
  // than a subresource, and it is the only one that does not reach either of
  // the two paths that count for themselves -- ShotFetch for http(s),
  // ShotURLLoader for a file: subresource. Without this a local page reported
  // `requests: 0` while the identical page over http reported 1, which is a
  // difference in the accounting and not in what happened.
  if (CaptureContext* context = CaptureContext::Current()) {
    context->RecordResource(/*from_cache=*/false, /*failed=*/false,
                            static_cast<int64_t>(input.body.size()));
  }
  // From the extension, because there is no server to say. This is how an SVG
  // or an XHTML file gets the parser it needs instead of being fed to the HTML
  // one, and net's table is the same one the file: protocol handler uses.
  std::string mime_type;
  if (net::GetMimeTypeFromFile(path, &mime_type)) {
    input.mime_type = mime_type;
  }
  return input;
}

// Fetches the top-level document over the network, blocking until it arrives.
//
// Blocking is what the shape of the pipeline asks for: Capture() is called from
// a request loop that has nothing else to do until this screenshot is finished,
// and ForceSynchronousDocumentInstall needs the whole body anyway. The run loop
// is what lets //net make progress in the meantime.
//
// The caller's extra headers are not a parameter here: ShotFetch takes them
// off the CaptureContext, which is also how the subresources get them. Passing
// them down this path as well would be a second way to say the same thing,
// and the two would eventually disagree about the same-origin rule.
base::expected<RenderInput, std::string> FetchDocument(
    const GURL& url,
    base::TimeDelta timeout) {
  if (!ShotNetwork::Get()) {
    return base::unexpected(
        "the network stack is not up, so only file: URLs can be rendered");
  }

  base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
  FetchResult result;
  bool answered = false;

  ShotFetch fetch;
  fetch.Start(
      url, net::HttpRequestHeaders(), url::Origin::Create(url),
      base::BindOnce(
          [](base::RunLoop* loop, FetchResult* out, bool* answered,
             FetchResult in) {
            *out = std::move(in);
            *answered = true;
            loop->Quit();
          },
          &run_loop, &result, &answered));

  // The deadline is enforced here rather than inside ShotFetch because it is
  // the caller's timeout: the same number also bounds how long the subresources
  // get, and one clock for the whole request is easier to reason about than
  // two. Quitting the loop drops `fetch`, which cancels the request.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE, run_loop.QuitClosure(), timeout);
  run_loop.Run();

  if (!answered) {
    return base::unexpected(base::StrCat(
        {"fetching ", url.spec(), " timed out after ",
         base::NumberToString(timeout.InMilliseconds()), "ms"}));
  }
  if (result.net_error != net::OK) {
    return base::unexpected(
        base::StrCat({"could not fetch ", url.spec(), ": ",
                      net::ErrorToShortString(result.net_error)}));
  }

  RenderInput input;
  // The final URL, not the requested one: everything the document references
  // relatively resolves against where it actually came from.
  input.url = result.final_url;
  input.body = std::move(result.body);
  input.charset = result.charset;
  // A server that sends no Content-Type gets the same treatment a browser
  // gives it for a top-level load: parse it as HTML.
  input.mime_type = result.mime_type.empty() ? "text/html" : result.mime_type;
  input.http_status = result.http_status;

  // A 4xx or 5xx is still a document, and photographing the error page is the
  // truthful answer to "what does this URL look like". The status is logged so
  // that a blank picture has something to explain it.
  if (result.http_status >= 400) {
    LOG(WARNING) << "shot: " << url.spec() << " answered HTTP "
                 << result.http_status;
  }
  return input;
}

// Everything a capture does before and after rendering: the context, the
// headers, the fetch of the document, the statistics on the way out. `render`
// is the part that differs between one image and several, and it is handed
// the document and the context to time itself against.
base::expected<void, std::string> WithDocument(
    ShotRuntime& runtime,
    const ScreenshotRequest& request,
    CaptureStats* out_stats,
    base::FunctionRef<base::expected<void, std::string>(RenderInput&,
                                                        CaptureContext&)>
        render) {
  const base::TimeTicks started = base::TimeTicks::Now();

  // Up before the target is even resolved, so that a request refused for its
  // scheme still reports the timings it did spend. It comes down when this
  // function returns, on every path including the failures.
  CaptureContext capture;

  int load_flags = 0;
  // The string was checked at parse time, so a failure here would mean the two
  // tables disagree rather than that the caller sent something odd.
  CHECK(CacheModeToLoadFlags(request.cache, &load_flags))
      << "unmapped cache mode " << request.cache;
  capture.set_load_flags(load_flags);

  // Delivered on the way out whatever happened, because the interesting case
  // for statistics is often the failure: forty subresources fetched and the
  // one that mattered timed out.
  base::ScopedClosureRunner deliver_stats(base::BindOnce(
      [](CaptureContext* capture, CaptureStats* out, base::TimeTicks started) {
        if (!out) {
          return;
        }
        capture->stats().total = base::TimeTicks::Now() - started;
        *out = capture->stats();
      },
      base::Unretained(&capture), out_stats, started));

  auto url = ResolveTarget(request.file);
  if (!url.has_value()) {
    return base::unexpected(url.error());
  }

  if (!request.headers.empty()) {
    net::HttpRequestHeaders headers;
    for (const auto& [name, value] : request.headers) {
      headers.SetHeader(name, value);
    }
    capture.SetExtraHeaders(std::move(headers), url::Origin::Create(*url));
  }

  const base::TimeTicks fetch_started = base::TimeTicks::Now();
  base::expected<RenderInput, std::string> input =
      base::unexpected(std::string());
  if (url->SchemeIsFile()) {
    input = ReadLocalDocument(*url);
  } else if (IsNetworkScheme(*url)) {
    input = FetchDocument(*url, base::Milliseconds(request.timeout_ms));
  } else {
    return base::unexpected(base::StrCat(
        {url->scheme(),
         ": is not a scheme this renderer can load; use http, https, file, or "
         "a local path"}));
  }
  capture.stats().fetch = base::TimeTicks::Now() - fetch_started;
  if (!input.has_value()) {
    return base::unexpected(input.error());
  }
  capture.stats().final_url = input->url.spec();
  capture.stats().http_status = input->http_status;

  const base::TimeTicks render_started = base::TimeTicks::Now();
  auto rendered = render(*input, capture);
  // The renderer reported its own encode time into the same stats block on
  // its way out, so what is left over here is parse, subresources, style,
  // layout and paint -- which is what `render` is documented to be.
  capture.stats().render =
      (base::TimeTicks::Now() - render_started) - capture.stats().encode;
  return rendered;
}

}  // namespace

base::expected<CaptureResult, std::string> Capture(
    ShotRuntime& runtime,
    const ScreenshotRequest& request,
    CaptureStats* out_stats) {
  CaptureResult result;
  auto captured = WithDocument(
      runtime, request, out_stats,
      [&](RenderInput& input,
          CaptureContext&) -> base::expected<void, std::string> {
        auto rendered = runtime.renderer().Render(input, request);
        if (!rendered.has_value()) {
          return base::unexpected(rendered.error());
        }
        result.image = std::move(rendered->bytes);
        result.size = rendered->size;
        result.wrote_path = !rendered->path.empty();
        return base::ok();
      });
  if (!captured.has_value()) {
    return base::unexpected(captured.error());
  }
  return result;
}

base::expected<std::vector<DeliveredTile>, std::string> CaptureTiles(
    ShotRuntime& runtime,
    const ScreenshotRequest& request,
    CaptureStats* out_stats) {
  if (!request.tile.has_value()) {
    return base::unexpected("a tiles capture needs tile.height");
  }
  // Checked before anything is fetched: a path that would have every tile
  // overwrite the last is a mistake worth catching in the first millisecond.
  const bool to_path = !request.path.empty();
  if (to_path && request.path.find("{n}") == std::string::npos) {
    return base::unexpected(
        "path for a tiles capture must contain {n}, which becomes each "
        "tile's number: page-{n}.png writes page-1.png, page-2.png, ...");
  }

  std::vector<DeliveredTile> tiles;
  auto captured = WithDocument(
      runtime, request, out_stats,
      [&](RenderInput& input,
          CaptureContext&) -> base::expected<void, std::string> {
        return runtime.renderer().RenderTiles(
            input, request,
            base::BindRepeating(
                [](std::vector<DeliveredTile>* tiles, bool to_path,
                   const std::string& path_template,
                   EncodedTile tile) -> base::expected<void, std::string> {
                  DeliveredTile delivered;
                  delivered.region = tile.region;
                  delivered.size = tile.size;
                  if (!tile.path.empty()) {
                    // The engine streamed the tile into the file itself.
                    delivered.path = std::move(tile.path);
                    tiles->push_back(std::move(delivered));
                    return base::ok();
                  }
                  if (!to_path) {
                    delivered.image = std::move(tile.bytes);
                    tiles->push_back(std::move(delivered));
                    return base::ok();
                  }
                  std::string path = path_template;
                  base::ReplaceSubstringsAfterOffset(
                      &path, 0, "{n}",
                      base::NumberToString(tiles->size() + 1));
                  if (!base::WriteFile(base::FilePath::FromUTF8Unsafe(path),
                                       tile.bytes)) {
                    return base::unexpected("could not write " + path);
                  }
                  delivered.path = std::move(path);
                  tiles->push_back(std::move(delivered));
                  return base::ok();
                },
                &tiles, to_path, request.path));
      });
  if (!captured.has_value()) {
    return base::unexpected(captured.error());
  }
  return tiles;
}

base::expected<CaptureResult, std::string> CaptureAndDeliver(
    ShotRuntime& runtime,
    const ScreenshotRequest& request,
    CaptureStats* out_stats) {
  auto result = Capture(runtime, request, out_stats);
  if (!result.has_value()) {
    return base::unexpected(result.error());
  }
  if (request.path.empty() || result->wrote_path) {
    // Nothing named, or the engine streamed the image into the file itself.
    return result;
  }

  // The engine handed the bytes back after all: write them. FromUTF8Unsafe
  // rather than a validated conversion because the path came in as UTF-8 JSON
  // and there is nothing to validate it against; a path that is not a path
  // fails at the write, which is where the caller can be told about it by
  // name.
  const base::FilePath path = base::FilePath::FromUTF8Unsafe(request.path);
  if (!base::WriteFile(path, result->image)) {
    return base::unexpected("could not write " + request.path);
  }
  result->image = Bytes();
  result->wrote_path = true;
  return result;
}

}  // namespace shot
