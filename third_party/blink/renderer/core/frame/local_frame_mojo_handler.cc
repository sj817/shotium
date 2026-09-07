// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/local_frame_mojo_handler.h"

#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/platform/browser_interface_broker_proxy.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"

namespace blink {

LocalFrameMojoHandler::LocalFrameMojoHandler(blink::LocalFrame& frame)
    : frame_(frame) {
  frame.GetRemoteNavigationAssociatedInterfaces()->GetInterface(
      back_forward_cache_controller_host_remote_.BindNewEndpointAndPassReceiver(
          frame.GetTaskRunner(TaskType::kInternalDefault)));
  frame.GetBrowserInterfaceBroker().GetInterface(
      non_associated_local_frame_host_remote_.BindNewPipeAndPassReceiver(
          frame.GetTaskRunner(TaskType::kInternalHighPriorityLocalFrame)));

  frame.GetRemoteNavigationAssociatedInterfaces()->GetInterface(
      local_frame_host_remote_.BindNewEndpointAndPassReceiver(
          frame.GetTaskRunner(TaskType::kInternalDefault)));
}

void LocalFrameMojoHandler::Trace(Visitor* visitor) const {
  visitor->Trace(frame_);
  visitor->Trace(back_forward_cache_controller_host_remote_);
  visitor->Trace(reporting_service_);
  visitor->Trace(local_frame_host_remote_);
  visitor->Trace(non_associated_local_frame_host_remote_);
}

mojom::blink::BackForwardCacheControllerHost&
LocalFrameMojoHandler::BackForwardCacheControllerHostRemote() {
  return *back_forward_cache_controller_host_remote_.get();
}

mojom::blink::ReportingServiceProxy* LocalFrameMojoHandler::ReportingService() {
  if (!reporting_service_.is_bound()) {
    frame_->GetBrowserInterfaceBroker().GetInterface(
        reporting_service_.BindNewPipeAndPassReceiver(
            frame_->GetTaskRunner(TaskType::kInternalDefault)));
  }
  return reporting_service_.get();
}

}  // namespace blink
