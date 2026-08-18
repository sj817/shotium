// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// This file exposes services from the GPU process to the browser.

#include "content/gpu/gpu_child_thread.h"

namespace content {

void GpuChildThread::BindServiceInterface(
    mojo::GenericPendingReceiver receiver) {
  // VizMain is the only service the GPU process still hosts. The media service
  // and the shape detection service were removed with their implementations,
  // so a receiver for anything else is dropped here and the browser sees the
  // pipe close.
  if (auto viz_receiver = receiver.As<viz::mojom::VizMain>()) {
    viz_main_.Bind(std::move(viz_receiver));
  }
}

}  // namespace content
