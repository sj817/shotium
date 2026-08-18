/*
 * Copyright (C) 2008, 2010 Apple Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/frame/location.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/csp/content_security_policy.h"
#include "third_party/blink/renderer/core/frame/dom_window.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/remote_dom_window.h"
#include "third_party/blink/renderer/core/loader/frame_load_request.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/url/dom_origin.h"
#include "third_party/blink/renderer/core/url/url_utils_read_only.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"

namespace blink {

Location::Location(DOMWindow* dom_window) : dom_window_(dom_window) {}

void Location::Trace(Visitor* visitor) const {
  visitor->Trace(dom_window_);
  visitor->Trace(ancestor_origins_list_);
  ScriptWrappable::Trace(visitor);
}

inline const KURL& Location::Url() const {
  const KURL& url = GetDocument()->Url();
  if (!url.IsValid()) {
    // Use "about:blank" while the page is still loading (before we have a
    // frame).
    return BlankUrl();
  }

  return url;
}

String Location::href() const {
  return Url().StrippedForUseAsHref();
}

String Location::protocol() const {
  return UrlUtilsReadOnly::protocol(Url());
}

String Location::host() const {
  return UrlUtilsReadOnly::host(Url());
}

String Location::hostname() const {
  return UrlUtilsReadOnly::hostname(Url());
}

String Location::port() const {
  return UrlUtilsReadOnly::port(Url());
}

String Location::pathname() const {
  return UrlUtilsReadOnly::pathname(Url());
}

String Location::search() const {
  return UrlUtilsReadOnly::search(Url());
}

String Location::origin() const {
  return UrlUtilsReadOnly::origin(Url());
}

DOMStringList* Location::ancestorOrigins() {
  if (!IsAttached()) {
    if (!ancestor_origins_list_ || !ancestor_origins_list_->IsEmpty() ||
        !RuntimeEnabledFeatures::AncestorOriginsStoredOnDocumentEnabled()) {
      ancestor_origins_list_ = MakeGarbageCollected<DOMStringList>();
    }
    return ancestor_origins_list_.Get();
  }

  if (!ancestor_origins_list_ ||
      !RuntimeEnabledFeatures::AncestorOriginsStoredOnDocumentEnabled()) {
    ancestor_origins_list_ = MakeGarbageCollected<DOMStringList>();
    for (Frame* frame = dom_window_->GetFrame()->Tree().Parent(); frame;
         frame = frame->Tree().Parent()) {
      ancestor_origins_list_->Append(
          frame->GetSecurityContext()->GetSecurityOrigin()->ToString());
    }
  }
  return ancestor_origins_list_.Get();
}

String Location::toString() const {
  return href();
}

String Location::hash() const {
  return UrlUtilsReadOnly::hash(Url());
}

void Location::reload() {
  if (!IsAttached())
    return;
  if (GetDocument()->Url().ProtocolIsJavaScript())
    return;
  // reload() is not cross-origin accessible, so |dom_window_| will always be
  // local.
  To<LocalDOMWindow>(dom_window_.Get())
      ->GetFrame()
      ->Reload(WebFrameLoadType::kReload);
}

Document* Location::GetDocument() const {
  return To<LocalDOMWindow>(dom_window_.Get())->document();
}

bool Location::IsAttached() const {
  return dom_window_->GetFrame();
}

}  // namespace blink
