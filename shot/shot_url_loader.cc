// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_url_loader.h"

#include <string>
#include <utility>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "mojo/public/cpp/system/data_pipe.h"
#include "mojo/public/cpp/system/string_data_source.h"
#include "net/base/filename_util.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "services/network/public/cpp/resource_request.h"
#include "shot/shot_capture_context.h"
#include "third_party/blink/public/platform/resource_load_info_notifier_wrapper.h"
#include "third_party/blink/public/platform/web_string.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/renderer/platform/loader/fetch/url_loader/url_loader_client.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/shared_buffer.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace shot {
namespace {

// The MIME type a file: response should claim, from its extension. Blink will
// sniff the bytes for images anyway, but stylesheets and fonts are dispatched
// on the declared type, so getting this wrong silently drops them.
blink::String MimeTypeForPath(const base::FilePath& path) {
  const base::FilePath::StringType ext = path.FinalExtension();
  if (ext.empty()) {
    return blink::String();
  }
  // FinalExtension() keeps the leading dot.
  const std::string without_dot =
      base::FilePath(ext).AsUTF8Unsafe().substr(1);
  return blink::MIMETypeRegistry::GetWellKnownMIMETypeForExtension(
      blink::String::FromUtf8(without_dot));
}

// Reads one file: URL. Returns false and leaves `error` set on failure.
bool ReadFileURL(const GURL& url,
                 blink::WebURLResponse& response,
                 std::string& contents,
                 std::optional<blink::WebURLError>& error) {
  // net::FileURLToFilePath, not the URL's path component. On Windows the path
  // of file:///D:/x/y.png is "/D:/x/y.png" -- with a leading slash, and with
  // percent-escapes still in it -- so treating it as a filename produced
  // "\D:\x\y.png", which does not exist. Every subresource in the corpus failed
  // this way while the main document, which main.cc converts with the same
  // net:: helper, loaded fine.
  // A file: resource never reaches ShotFetch, so the accounting every http(s)
  // request gets for free in Finish() has to be done by hand here. It is
  // counted for the same reason: a caller looking at `failed` wants to know
  // that an <img src="missing.png"> did not load, and cares no more that it
  // was a local file than blink does.
  CaptureContext* capture = CaptureContext::Current();

  base::FilePath path;
  if (!net::FileURLToFilePath(url, &path)) {
    if (capture) {
      capture->RecordResource(/*from_cache=*/false, /*failed=*/true, 0);
    }
    error = blink::WebURLError(net::ERR_INVALID_URL,
                               blink::WebURL(blink::KURL(url)));
    return false;
  }
  if (!base::ReadFileToString(path, &contents)) {
    LOG(INFO) << "shot: load FAILED (unreadable) " << url.spec();
    if (capture) {
      capture->RecordResource(/*from_cache=*/false, /*failed=*/true, 0);
    }
    error = blink::WebURLError(net::ERR_FILE_NOT_FOUND,
                               blink::WebURL(blink::KURL(url)));
    return false;
  }
  if (capture) {
    capture->RecordResource(/*from_cache=*/false, /*failed=*/false,
                            static_cast<int64_t>(contents.size()));
  }

  response = blink::WebURLResponse(blink::WebURL(blink::KURL(url)));
  response.SetMimeType(blink::WebString(MimeTypeForPath(path)));
  response.SetHttpStatusCode(200);
  response.SetExpectedContentLength(
      static_cast<int64_t>(contents.size()));
  LOG(INFO) << "shot: load ok " << url.spec() << " (" << contents.size()
            << " bytes, " << MimeTypeForPath(path).Utf8() << ")";
  return true;
}

// Turns what //net reports into what blink's fetch expects.
//
// The URL is the one the bytes actually came from, after redirects. blink is
// never told about the redirect chain -- ShotFetch follows it internally -- so
// this is the only place the final URL is communicated, and a stylesheet
// resolving its relative url() references depends on it being right.
blink::WebURLResponse BuildHttpResponse(const FetchResult& result) {
  blink::WebURLResponse response(blink::WebURL(blink::KURL(result.final_url)));
  response.SetHttpStatusCode(result.http_status);
  response.SetMimeType(blink::WebString::FromUtf8(result.mime_type));
  response.SetTextEncodingName(blink::WebString::FromUtf8(result.charset));
  response.SetExpectedContentLength(static_cast<int64_t>(result.body.size()));
  response.SetEncodedBodyLength(result.body.size());
  response.SetWasCached(result.was_cached);
  response.SetNetworkAccessed(!result.was_cached);

  if (result.headers) {
    response.SetHttpStatusText(
        blink::WebString::FromUtf8(result.headers->GetStatusText()));
    // Every header, verbatim. blink reads more of them than it looks like:
    // Content-Type decides which parser runs, Cache-Control and friends decide
    // whether a second use of the same URL reuses the Resource, and
    // Access-Control-Allow-Origin decides whether a cross-origin font is
    // usable.
    size_t iter = 0;
    std::string name;
    std::string value;
    while (result.headers->EnumerateHeaderLines(&iter, &name, &value)) {
      response.AddHttpHeaderField(blink::WebString::FromUtf8(name),
                                  blink::WebString::FromUtf8(value));
    }
  }
  return response;
}

bool IsNetworkScheme(const GURL& url) {
  return url.SchemeIs(url::kHttpScheme) || url.SchemeIs(url::kHttpsScheme);
}

// Which origin a request is attributed to, for the cache and cookie
// partitions. The initiator is what blink filled in; falling back to the
// request's own origin keeps a request that arrives without one in its own
// partition rather than in everybody's.
url::Origin InitiatorFor(const network::ResourceRequest& request) {
  if (request.request_initiator.has_value() &&
      !request.request_initiator->opaque()) {
    return *request.request_initiator;
  }
  return url::Origin::Create(request.url);
}

}  // namespace

ShotURLLoader::ShotURLLoader() = default;
ShotURLLoader::~ShotURLLoader() = default;

void ShotURLLoader::LoadSynchronously(
    std::unique_ptr<network::ResourceRequest> request,
    scoped_refptr<const blink::SecurityOrigin> top_frame_origin,
    bool download_to_blob,
    bool no_mime_sniffing,
    base::TimeDelta timeout_interval,
    blink::URLLoaderClient* client,
    blink::WebURLResponse& response,
    std::optional<blink::WebURLError>& error,
    scoped_refptr<blink::SharedBuffer>& data,
    int64_t& encoded_data_length,
    uint64_t& encoded_body_length,
    scoped_refptr<blink::BlobDataHandle>& downloaded_blob,
    std::unique_ptr<blink::ResourceLoadInfoNotifierWrapper>
        resource_load_info_notifier_wrapper) {
  const GURL url = request->url;

  std::string contents;
  if (url.SchemeIsFile()) {
    if (!ReadFileURL(url, response, contents, error)) {
      return;
    }
  } else if (IsNetworkScheme(url)) {
    // A nested run loop, because the caller is blocked on this returning and
    // //net has nowhere else to run. blink asks for a synchronous load only for
    // synchronous XHR and a couple of import paths -- all script-driven, and
    // there is no script in this binary -- so this exists to be correct rather
    // than to be fast.
    base::RunLoop run_loop(base::RunLoop::Type::kNestableTasksAllowed);
    FetchResult result;
    ShotFetch fetch;
    fetch.Start(url, request->headers, InitiatorFor(*request),
                base::BindOnce(
                    [](base::RunLoop* loop, FetchResult* out, FetchResult in) {
                      *out = std::move(in);
                      loop->Quit();
                    },
                    &run_loop, &result));
    run_loop.Run();

    if (result.net_error != net::OK) {
      error =
          blink::WebURLError(result.net_error, blink::WebURL(blink::KURL(url)));
      return;
    }
    response = BuildHttpResponse(result);
    contents = std::move(result.body);
  } else {
    // data: never reaches a loader -- blink decodes it inline -- so anything
    // that gets here and is neither a file nor a network URL is a scheme this
    // binary does not implement. Saying so is the honest answer; returning an
    // empty 200 would produce a page that renders wrong with nothing to point
    // at.
    error = blink::WebURLError(net::ERR_DISALLOWED_URL_SCHEME,
                               blink::WebURL(blink::KURL(url)));
    return;
  }

  data = blink::SharedBuffer::Create(base::span(contents));
  encoded_data_length = static_cast<int64_t>(contents.size());
  encoded_body_length = contents.size();
}

void ShotURLLoader::LoadAsynchronously(
    std::unique_ptr<network::ResourceRequest> request,
    scoped_refptr<const blink::SecurityOrigin> top_frame_origin,
    bool no_mime_sniffing,
    std::unique_ptr<blink::ResourceLoadInfoNotifierWrapper>
        resource_load_info_notifier_wrapper,
    blink::CodeCacheHost* code_cache_host,
    blink::URLLoaderClient* client) {
  const GURL url = request->url;
  LOG(INFO) << "shot: request " << url.spec();

  if (IsNetworkScheme(url)) {
    // ShotFetch already answers asynchronously, so unlike the file path there
    // is no need to post: the client cannot be called back before this returns.
    fetch_ = std::make_unique<ShotFetch>();
    fetch_->Start(url, request->headers, InitiatorFor(*request),
                  base::BindOnce(&ShotURLLoader::OnFetched,
                                 weak_factory_.GetWeakPtr(), client));
    return;
  }

  // Reading a local file is fast enough to do inline, but the client is not
  // prepared to be called back before this returns -- ResourceLoader sets up
  // its state after asking for the load. So the delivery is posted.
  base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&ShotURLLoader::DeliverFile,
                                weak_factory_.GetWeakPtr(), url, client));
}

void ShotURLLoader::DeliverFile(const GURL& url,
                                blink::URLLoaderClient* client) {
  blink::WebURLResponse response;
  std::string contents;
  std::optional<blink::WebURLError> error;
  if (!url.SchemeIsFile()) {
    error = blink::WebURLError(net::ERR_DISALLOWED_URL_SCHEME,
                               blink::WebURL(blink::KURL(url)));
  } else {
    ReadFileURL(url, response, contents, error);
  }
  if (error) {
    client->DidFail(*error, base::TimeTicks::Now(), 0, 0, 0);
    return;
  }
  DeliverBody(client, response, std::move(contents));
}

void ShotURLLoader::OnFetched(blink::URLLoaderClient* client,
                              FetchResult result) {
  if (result.net_error != net::OK) {
    LOG(INFO) << "shot: load FAILED " << result.final_url.spec() << " ("
              << net::ErrorToShortString(result.net_error) << ")";
    client->DidFail(
        blink::WebURLError(result.net_error,
                           blink::WebURL(blink::KURL(result.final_url))),
        base::TimeTicks::Now(), 0, 0, 0);
    return;
  }
  LOG(INFO) << "shot: load ok " << result.final_url.spec() << " ("
            << result.body.size() << " bytes, " << result.mime_type
            << (result.was_cached ? ", from cache" : "") << ")";
  DeliverBody(client, BuildHttpResponse(result), std::move(result.body));
}

void ShotURLLoader::DeliverBody(blink::URLLoaderClient* client,
                                const blink::WebURLResponse& response,
                                std::string contents) {
  // The body goes down a mojo data pipe, which is how the network service
  // delivers one. That is not ceremony: ResourceLoader::DidReceiveResponse only
  // takes the streaming path -- the one that hands its Resource
  // span<const char> chunks -- when the body arrives as a pipe. Handing it a
  // SegmentedBuffer instead routes the body to the background-response path,
  // and ImageResource::AppendData CHECKs that it is never called that way,
  // because there is no BackgroundResponseProcessor for images. Every <img> in
  // the corpus took the process down that way.
  mojo::ScopedDataPipeProducerHandle producer;
  mojo::ScopedDataPipeConsumerHandle consumer;
  if (mojo::CreateDataPipe(nullptr, producer, consumer) != MOJO_RESULT_OK) {
    client->DidFail(
        blink::WebURLError(net::ERR_INSUFFICIENT_RESOURCES,
                           blink::WebURL(response.CurrentRequestUrl())),
        base::TimeTicks::Now(), 0, 0, 0);
    return;
  }

  body_ = std::move(contents);
  const int64_t size = static_cast<int64_t>(body_.size());
  client->DidReceiveResponse(response, std::move(consumer),
                             /*cached_metadata=*/std::nullopt);

  // DataPipeProducer owns the chunking and the writable-watcher loop, so a body
  // larger than the pipe's capacity is written in as many passes as it takes
  // rather than silently truncated. `body_` outlives the write -- it is a
  // member and this loader is kept alive by blink until the load ends -- so the
  // source may reference it instead of copying it.
  body_producer_ = std::make_unique<mojo::DataPipeProducer>(std::move(producer));
  body_producer_->Write(
      std::make_unique<mojo::StringDataSource>(
          base::span(body_),
          mojo::StringDataSource::AsyncWritingMode::
              STRING_STAYS_VALID_UNTIL_COMPLETION),
      base::BindOnce(&ShotURLLoader::OnBodyWritten, weak_factory_.GetWeakPtr(),
                     client, size));
}

void ShotURLLoader::OnBodyWritten(blink::URLLoaderClient* client,
                                  int64_t size,
                                  MojoResult result) {
  // Dropping the producer closes the pipe, which is how the consumer learns
  // the body is complete. DidFinishLoading has to come after that, not before.
  body_producer_.reset();
  if (result != MOJO_RESULT_OK) {
    LOG(ERROR) << "shot: writing the response body failed (mojo result "
               << result << ")";
    return;
  }
  client->DidFinishLoading(base::TimeTicks::Now(), size,
                           static_cast<uint64_t>(size), size);
}

scoped_refptr<base::SingleThreadTaskRunner>
ShotURLLoader::GetTaskRunnerForBodyLoader() {
  return base::SingleThreadTaskRunner::GetCurrentDefault();
}

}  // namespace shot
