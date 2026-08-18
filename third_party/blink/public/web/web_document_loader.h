/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_DOCUMENT_LOADER_H_
#define THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_DOCUMENT_LOADER_H_

#include <memory>

#include "services/network/public/mojom/ip_address_space.mojom-shared.h"
#include "third_party/blink/public/mojom/loader/code_cache.mojom-shared.h"
#include "third_party/blink/public/platform/cross_variant_mojo_util.h"
#include "third_party/blink/public/platform/web_archive_info.h"
#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/web/web_navigation_type.h"

namespace blink {

class WebURL;
class WebURLResponse;

namespace mojom {
enum class FetchCacheMode : int32_t;
}  // namespace mojom

// An interface to expose the blink::DocumentLoader to the content layer,
// including SetExtraData() and GetExtraData() to allow the content layer to
// store data that isn't relevant to Blink.
class BLINK_EXPORT WebDocumentLoader {
 public:
  class ExtraData {
   public:
    virtual ~ExtraData() = default;
    virtual std::unique_ptr<ExtraData> Clone() = 0;
  };

  static bool WillLoadUrlAsEmpty(const WebURL&);

  // Returns the http referrer of original request which initited this load.
  virtual WebString OriginalReferrer() const = 0;

  // Returns the url corresponding to this load. It may also be a url
  // specified by a redirect that was followed.
  virtual WebURL GetUrl() const = 0;

  // Returns the http method of the request corresponding to this load.
  virtual WebString HttpMethod() const = 0;

  // Returns the http referrer of the request corresponding to this load.
  virtual WebString Referrer() const = 0;

  // Returns the response associated with this datasource.
  virtual const WebURLResponse& GetWebResponse() const = 0;

  // When this datasource was created as a result of WebFrame::loadData,
  // there may be an associated unreachableURL.
  virtual bool HasUnreachableURL() const = 0;
  virtual WebURL UnreachableWebURL() const = 0;

  // Returns whether the navigation associated with this datasource is a
  // client redirect.
  virtual bool IsClientRedirect() const = 0;

  // Returns whether the navigation associated with this datasource should
  // replace the current history item.
  virtual bool ReplacesCurrentHistoryItem() const = 0;

  // The type of navigation that triggered the creation of this datasource.
  virtual WebNavigationType GetNavigationType() const = 0;

  // Extra data associated with this DocumentLoader.
  // Setting extra data will cause any existing extra data to be deleted.
  virtual ExtraData* GetExtraData() const = 0;
  virtual std::unique_ptr<ExtraData> CloneExtraData() = 0;
  virtual void SetExtraData(std::unique_ptr<ExtraData>) = 0;

  // SetSubresourceFilter() was here. It let the embedder inject a filter with
  // the final say over every subresource load. WebDocumentSubresourceFilter has
  // no implementation left in this build and nothing ever called this.

  // Allows the embedder to set and return the service worker provider
  // Set/GetServiceWorkerNetworkProvider() were here: the embedder handed the
  // loader the object that let a controlling service worker intercept this
  // document's subresource requests. Service workers are cut.

  // Can be used to temporarily suspend feeding the parser with new data. The
  // parser will be allowed to read new data when ResumeParser() is called the
  // same number of time than BlockParser().
  virtual void BlockParser() = 0;
  virtual void ResumeParser() = 0;

  // Returns true if the document is an MHTML archive. When true,
  // GetArchiveInfo() may be called to find the result of loading and, if
  // loading was successful, more information about the archive. Note that this
  // can return true even if archive loading ended up failing.
  virtual bool HasBeenLoadedAsWebArchive() const = 0;

  // Returns archive info for the archive.
  virtual WebArchiveInfo GetArchiveInfo() const = 0;

  // Whether the last navigation (cross-document or same-document) that
  // committed in this WebDocumentLoader had transient activation.
  virtual bool LastNavigationHadTransientUserActivation() const = 0;

  // Sets the CodeCacheHosts for this loader.
  virtual void SetCodeCacheHost(
      CrossVariantMojoRemote<mojom::CodeCacheHostInterfaceBase> code_cache_host,
      CrossVariantMojoRemote<mojom::CodeCacheHostInterfaceBase>
          code_cache_host_for_background) = 0;

  // Whether the frame holding this document has loaded a document that is not
  // an initial empty document.
  virtual bool HasLoadedNonInitialEmptyDocument() const = 0;

  // Returns whether the navigation associated with this datasource is for a
  // frame discard operation, performed with the intention to clear away
  // associated resources.
  virtual bool IsForDiscard() const = 0;

 protected:
  ~WebDocumentLoader() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_DOCUMENT_LOADER_H_
