// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/loader/frame_resource_fetcher_properties.h"

#include "third_party/blink/public/platform/web_effective_connection_type.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/loader/document_loader.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/core/script/fetch_client_settings_object_impl.h"
#include "third_party/blink/renderer/platform/loader/fetch/fetch_client_settings_object.h"
#include "third_party/blink/renderer/platform/network/network_state_notifier.h"

namespace blink {

FrameResourceFetcherProperties::FrameResourceFetcherProperties(
    DocumentLoader& document_loader,
    Document& document)
    : document_loader_(document_loader),
      document_(document),
      fetch_client_settings_object_(
          MakeGarbageCollected<FetchClientSettingsObjectImpl>(
              *document.domWindow())) {}

void FrameResourceFetcherProperties::Trace(Visitor* visitor) const {
  visitor->Trace(document_loader_);
  visitor->Trace(document_);
  visitor->Trace(fetch_client_settings_object_);
  ResourceFetcherProperties::Trace(visitor);
}

bool FrameResourceFetcherProperties::IsOutermostMainFrame() const {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  return frame->IsOutermostMainFrame();
}

mojom::ControllerServiceWorkerMode
FrameResourceFetcherProperties::GetControllerServiceWorkerMode() const {
  // This asked the document's service worker network provider which mode it was
  // in. Service workers are cut, so there is never a controller -- which is
  // exactly what the provider-is-null branch returned.
  return blink::mojom::ControllerServiceWorkerMode::kNoController;
}

int64_t FrameResourceFetcherProperties::ServiceWorkerId() const {
  // Only reachable with a controller, and GetControllerServiceWorkerMode()
  // above can no longer report one.
  NOTREACHED();
}

bool FrameResourceFetcherProperties::IsPaused() const {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  return frame->GetPage()->Paused();
}

LoaderFreezeMode FrameResourceFetcherProperties::FreezeMode() const {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  return frame->GetLoaderFreezeMode();
}

bool FrameResourceFetcherProperties::IsLoadComplete() const {
  return document_->LoadEventFinished();
}

bool FrameResourceFetcherProperties::ShouldBlockLoadingSubResource() const {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  return document_loader_ != frame->Loader().GetDocumentLoader();
}

bool FrameResourceFetcherProperties::IsSubframeDeprioritizationEnabled() const {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  Settings* settings = frame->GetSettings();
  if (!settings) {
    return false;
  }

  const WebEffectiveConnectionType max_effective_connection_type_threshold =
      settings->GetLowPriorityIframesThreshold();
  if (max_effective_connection_type_threshold <=
      WebEffectiveConnectionType::kTypeOffline) {
    return false;
  }

  const WebEffectiveConnectionType effective_connection_type =
      GetNetworkStateNotifier().EffectiveType();
  if (effective_connection_type <= WebEffectiveConnectionType::kTypeOffline) {
    return false;
  }

  if (effective_connection_type > max_effective_connection_type_threshold) {
    // Network is not slow enough.
    return false;
  }

  return true;
}

scheduler::FrameStatus FrameResourceFetcherProperties::GetFrameStatus() const {
  LocalFrame* frame = document_->GetFrame();
  DCHECK(frame);
  return scheduler::GetFrameStatus(frame->GetFrameScheduler());
}

int FrameResourceFetcherProperties::GetOutstandingThrottledLimit() const {
  return IsOutermostMainFrame() ? 3 : 2;
}

}  // namespace blink
