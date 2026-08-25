// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_capture.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "net/base/filename_util.h"
#include "net/base/mime_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "shot/shot_fetch.h"
#include "shot/shot_network.h"
#include "shot/shot_renderer.h"
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
    return base::unexpected(
        base::StrCat({"could not read ", path.AsUTF8Unsafe()}));
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

  // A 4xx or 5xx is still a document, and photographing the error page is the
  // truthful answer to "what does this URL look like". The status is logged so
  // that a blank picture has something to explain it.
  if (result.http_status >= 400) {
    LOG(WARNING) << "shot: " << url.spec() << " answered HTTP "
                 << result.http_status;
  }
  return input;
}

}  // namespace

base::expected<std::vector<uint8_t>, std::string> Capture(
    const ScreenshotRequest& request) {
  auto url = ResolveTarget(request.file);
  if (!url.has_value()) {
    return base::unexpected(url.error());
  }

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
  if (!input.has_value()) {
    return base::unexpected(input.error());
  }

  ShotRenderer renderer;
  return renderer.Render(*input, request);
}

base::expected<CaptureResult, std::string> CaptureAndDeliver(
    const ScreenshotRequest& request) {
  auto image = Capture(request);
  if (!image.has_value()) {
    return base::unexpected(image.error());
  }

  CaptureResult result;
  result.size = image->size();
  if (request.path.empty()) {
    result.image = std::move(*image);
    return result;
  }

  // FromUTF8Unsafe rather than a validated conversion because the path came in
  // as UTF-8 JSON and there is nothing to validate it against; a path that is
  // not a path fails at the write, which is where the caller can be told about
  // it by name.
  const base::FilePath path = base::FilePath::FromUTF8Unsafe(request.path);
  if (!base::WriteFile(path, *image)) {
    return base::unexpected("could not write " + request.path);
  }
  result.wrote_path = true;
  return result;
}

}  // namespace shot
