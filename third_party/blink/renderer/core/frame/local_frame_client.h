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
#include "base/unguessable_token.h"
#include "third_party/blink/public/common/user_agent/user_agent_metadata.h"
#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/frame_client.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace blink {

class AssociatedInterfaceProvider;
class Frame;
class HTMLFrameOwnerElement;
class LocalFrame;
class URLLoader;

class CORE_EXPORT LocalFrameClient : public FrameClient {
 public:
  ~LocalFrameClient() override = default;

  virtual base::UnguessableToken GetDevToolsFrameToken() const {
    return base::UnguessableToken::Create();
  }

  virtual void DispatchDidHandleOnloadEvents() {}
  virtual void DispatchDidFinishLoad() {}

  virtual void DidStopLoading() {}

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
