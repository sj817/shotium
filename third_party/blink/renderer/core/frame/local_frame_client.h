/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights
 * reserved.
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_CLIENT_H_

#include <memory>
#include <optional>

#include "base/notreached.h"
#include "base/time/time.h"
#include "base/unguessable_token.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "mojo/public/cpp/bindings/pending_associated_remote.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "services/network/public/mojom/content_security_policy.mojom-blink-forward.h"
#include "services/network/public/mojom/web_sandbox_flags.mojom-blink-forward.h"
#include "third_party/blink/public/common/tokens/tokens.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "third_party/blink/public/mojom/blob/blob_url_store.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/fenced_frame/fenced_frame.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/frame/triggering_event_info.mojom-blink-forward.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/public/platform/web_effective_connection_type.h"
#include "third_party/blink/public/web/web_frame_load_type.h"
#include "third_party/blink/public/web/web_history_commit_type.h"
#include "third_party/blink/public/web/web_navigation_params.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/frame_client.h"
#include "third_party/blink/renderer/core/frame/frame_types.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/loader/frame_loader_types.h"
#include "third_party/blink/renderer/core/loader/navigation_policy.h"
#include "third_party/blink/renderer/platform/weborigin/referrer.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace blink {

class AssociatedInterfaceProvider;
class DocumentLoader;
class HTMLFormElement;
class HTMLFrameOwnerElement;
class KURL;
class LocalDOMWindow;
class LocalFrame;
class ResourceRequest;
class ResourceResponse;
class SourceLocation;
class URLLoader;

class CORE_EXPORT LocalFrameClient : public FrameClient {
 public:
  ~LocalFrameClient() override = default;

  // GetWebContentCaptureClient() was here. It handed core the embedder's
  // WebContentCaptureClient, which content capture streamed on-screen text
  // to. ContentCaptureManager and its only caller in LocalFrame are cut, so
  // nothing asks for the client any more.

  virtual base::UnguessableToken GetDevToolsFrameToken() const {
    return base::UnguessableToken::Create();
  }

  virtual void WillBeDetached() {}
  virtual void DispatchFinalizeRequest(ResourceRequest&) {}
  virtual std::optional<KURL> DispatchWillSendRequest(
      const KURL& requested_url,
      const scoped_refptr<const SecurityOrigin>& requestor_origin,
      const net::SiteForCookies& site_for_cookies,
      bool has_redirect_info,
      const KURL& upstream_url) {
    return std::nullopt;
  }
  virtual void DispatchDidLoadResourceFromMemoryCache(
      const ResourceRequest&,
      const ResourceResponse&) {}

  virtual void DispatchDidHandleOnloadEvents() {}
  virtual void DispatchDidFinishLoad() {}

  virtual void BeginNavigation(
      const ResourceRequest&,
      const KURL& requestor_base_url,
      mojom::RequestContextFrameType,
      LocalDOMWindow* origin_window,
      DocumentLoader*,
      WebNavigationType,
      NavigationPolicy,
      WebFrameLoadType,
      mojom::blink::ForceHistoryPush,
      bool is_client_redirect,
      // TODO(crbug.com/1315802): Refactor _unfencedTop handling.
      bool is_unfenced_top_navigation,
      mojom::blink::TriggeringEventInfo,
      HTMLFormElement*,
      network::mojom::CSPDisposition
          should_check_main_world_content_security_policy,
      mojo::PendingRemote<mojom::blink::BlobURLToken>,
      base::TimeTicks input_start_time,
      base::TimeTicks actual_navigation_start,
      const String& href_translate,
      const LocalFrameToken* initiator_frame_token,
      const InitiatorStateToken& initiator_state_token,
      const DocumentToken& initiator_document_token,
      SourceLocation* source_location,
      bool is_container_initiated,
      bool has_rel_opener,
      mojo::PendingReceiver<
          mojom::blink::NavigationResumeDeferredCommitListener>
          resume_defer_commit_listener,
      std::optional<base::UnguessableToken> script_tool_invocation_id) {}

  virtual void DidStartLoading() {}
  virtual void DidStopLoading() {}

  virtual void DidCreateDocumentLoader(DocumentLoader*) {}

  virtual String UserAgentOverride() { return ""; }
  virtual String UserAgent() { return ""; }
  virtual std::optional<blink::UserAgentMetadata> UserAgentMetadata() {
    return blink::UserAgentMetadata();
  }

  virtual LocalFrame* CreateFrame(const AtomicString& name,
                                  HTMLFrameOwnerElement*) {
    return nullptr;
  }

  virtual WebContentSettingsClient* GetContentSettingsClient() {
    return nullptr;
  }

  unsigned BackForwardLength() override { return 0; }

  virtual AssociatedInterfaceProvider*
  GetRemoteNavigationAssociatedInterfaces() {
    return nullptr;
  }

  virtual scoped_refptr<network::SharedURLLoaderFactory>
  GetURLLoaderFactory() {
    NOTREACHED();
  }
  virtual std::unique_ptr<URLLoader> CreateURLLoaderForTesting() {
    return nullptr;
  }

  virtual Frame* FindFrame(const AtomicString& name) const {
    return nullptr;
  }

};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_CLIENT_H_
