/*
 * Copyright (C) 2012 Motorola Mobility Inc.
 * Copyright (C) 2013 Google Inc. All Rights Reserved.
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

#include "third_party/blink/renderer/core/fileapi/public_url_manager.h"

#include "base/check.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/platform/scheduler/main_thread/task_type_names.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {

PublicURLManager::PublicURLManager(ExecutionContext* execution_context)
    : ExecutionContextLifecycleObserver(execution_context),
      frame_url_store_(execution_context) {
  // This used to branch on whether execution_context was a LocalDOMWindow, a
  // WorkerGlobalScope, or a WorkletGlobalScope, binding frame_url_store_ or
  // worker_url_store_ accordingly. WorkerGlobalScope and WorkletGlobalScope
  // are gone -- this is a single-threaded screenshot process with no workers
  // or worklets -- so execution_context is always a LocalDOMWindow now; the
  // other branches (and the worker_url_store_ they bound) are deleted.
  auto* window = DynamicTo<LocalDOMWindow>(execution_context);
  CHECK(window);
  LocalFrame* frame = window->GetFrame();
  if (!frame) {
    is_stopped_ = true;
    return;
  }

  frame->GetRemoteNavigationAssociatedInterfaces()->GetInterface(
      frame_url_store_.BindNewEndpointAndPassReceiver(
          execution_context->GetTaskRunner(TaskType::kFileReading)));
}

// The PassKey<GlobalStorageAccessHandle> constructor used to let the Storage
// Access API hand this class an already-connected BlobURLStore remote from a
// worker context. GlobalStorageAccessHandle and workers are both gone in
// this single-threaded screenshot process, so that entry point (and its only
// caller) no longer exists; deleted along with it.

mojom::blink::BlobURLStore& PublicURLManager::GetBlobURLStore() {
  DCHECK(frame_url_store_.is_bound());
  return *frame_url_store_.get();
}

void PublicURLManager::Resolve(
    const KURL& url,
    mojo::PendingReceiver<network::mojom::blink::URLLoaderFactory>
        factory_receiver) {
  if (is_stopped_)
    return;

  DCHECK(url.ProtocolIs("blob"));

  GetBlobURLStore().ResolveAsURLLoaderFactory(url, std::move(factory_receiver));
}

void PublicURLManager::ResolveAsBlobURLToken(
    const KURL& url,
    mojo::PendingReceiver<mojom::blink::BlobURLToken> token_receiver,
    bool is_top_level_navigation) {
  if (is_stopped_)
    return;

  DCHECK(url.ProtocolIs("blob"));

  GetBlobURLStore().ResolveAsBlobURLToken(url, std::move(token_receiver),
                                          is_top_level_navigation);
}

void PublicURLManager::ContextDestroyed() {
  if (is_stopped_)
    return;

  is_stopped_ = true;
}

void PublicURLManager::Trace(Visitor* visitor) const {
  visitor->Trace(frame_url_store_);
  ExecutionContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
