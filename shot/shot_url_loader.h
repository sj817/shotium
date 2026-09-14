// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_URL_LOADER_H_
#define SHOT_SHOT_URL_LOADER_H_

#include <memory>
#include <string>

#include "shot/shot_fetch.h"
#include "third_party/blink/renderer/platform/loader/fetch/url_loader/url_loader.h"
#include "url/gurl.h"

namespace shot {

// Loads the subresources a document references: file: off the disk, http: and
// https: off the network.
//
// blink's ResourceFetcher asks its LocalFrameClient for a URLLoader whenever a
// document references a subresource -- an <img>, an @font-face, a
// <link rel=stylesheet>. Upstream that loader is a mojo pipe to the network
// service, which is a separate process wrapping //net. There is no such process
// here, so this talks to //net directly; see shot_network.h for why that is the
// whole of the network stack and not a reduced version of it.
//
// What it refuses is what this binary genuinely cannot serve -- blob:,
// filesystem:, anything backed by a browser-side store that does not exist --
// with a "not supported" error rather than an empty 200, because a silently
// empty response produces a picture that is wrong in a way nobody can see.
//
// Only LoadAsynchronously and LoadSynchronously are overridden. The rest of
// URLLoader's surface -- Freeze, DidChangePriority, the background-response
// hooks -- describes scheduling of a network pipe that does not exist, and the
// base class already does the right nothing.
class ShotURLLoader : public blink::URLLoader {
 public:
  ShotURLLoader();
  ShotURLLoader(const ShotURLLoader&) = delete;
  ShotURLLoader& operator=(const ShotURLLoader&) = delete;
  ~ShotURLLoader() override;

  // blink::URLLoader:
  void LoadSynchronously(
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
      scoped_refptr<blink::BlobDataHandle>& downloaded_blob) override;

  void LoadAsynchronously(
      std::unique_ptr<network::ResourceRequest> request,
      scoped_refptr<const blink::SecurityOrigin> top_frame_origin,
      bool no_mime_sniffing,
      blink::URLLoaderClient* client) override;

  scoped_refptr<base::SingleThreadTaskRunner> GetTaskRunnerForBodyLoader()
      override;

 private:
  // Reads a file: URL and delivers it. Posted rather than called inline: the
  // client is not prepared to be called back before LoadAsynchronously returns,
  // because ResourceLoader sets up its state after asking for the load.
  void DeliverFile(const GURL& url, blink::URLLoaderClient* client);
  // The other half, once ShotFetch has the bytes.
  void OnFetched(blink::URLLoaderClient* client, FetchResult result);
  // Common tail: hands the client the response, the body and the finish, in
  // that order and synchronously. `charge` is what those bytes cost against
  // the fetch budget, which this loader takes over for as long as it holds
  // them; a file: body cost nothing and passes an empty one.
  void DeliverBody(blink::URLLoaderClient* client,
                   const blink::WebURLResponse& response,
                   std::string contents,
                   FetchCharge charge);

  // Alive for the length of a network load; destroying it cancels the request.
  std::unique_ptr<ShotFetch> fetch_;

  base::WeakPtrFactory<ShotURLLoader> weak_factory_{this};
};

}  // namespace shot

#endif  // SHOT_SHOT_URL_LOADER_H_
