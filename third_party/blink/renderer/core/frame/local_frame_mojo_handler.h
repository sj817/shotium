// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_MOJO_HANDLER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_MOJO_HANDLER_H_

#include "third_party/blink/public/mojom/frame/back_forward_cache_controller.mojom-blink.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink.h"
#include "third_party/blink/public/mojom/reporting/reporting.mojom-blink.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/mojo/heap_mojo_associated_remote.h"
#include "third_party/blink/renderer/platform/mojo/heap_mojo_remote.h"

namespace blink {

class LocalFrame;

// Holds the outbound host remotes used by LocalFrame. Browser command
// receivers are not part of the static renderer.
class LocalFrameMojoHandler : public GarbageCollected<LocalFrameMojoHandler> {
 public:
  explicit LocalFrameMojoHandler(LocalFrame& frame);
  void Trace(Visitor* visitor) const;

  mojom::blink::LocalFrameHost& LocalFrameHostRemote() {
    return *local_frame_host_remote_.get();
  }

  mojom::blink::NonAssociatedLocalFrameHost&
  NonAssociatedLocalFrameHostRemote() {
    return *non_associated_local_frame_host_remote_.get();
  }

  mojom::blink::ReportingServiceProxy* ReportingService();
  mojom::blink::BackForwardCacheControllerHost&
  BackForwardCacheControllerHostRemote();

 private:
  Member<LocalFrame> frame_;
  HeapMojoAssociatedRemote<mojom::blink::BackForwardCacheControllerHost>
      back_forward_cache_controller_host_remote_{nullptr};
  HeapMojoRemote<mojom::blink::ReportingServiceProxy> reporting_service_{nullptr};
  HeapMojoAssociatedRemote<mojom::blink::LocalFrameHost>
      local_frame_host_remote_{nullptr};
  HeapMojoRemote<mojom::blink::NonAssociatedLocalFrameHost>
      non_associated_local_frame_host_remote_{nullptr};
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_MOJO_HANDLER_H_
