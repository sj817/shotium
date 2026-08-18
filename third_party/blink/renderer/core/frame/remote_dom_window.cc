// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/frame/remote_dom_window.h"

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/events/message_event.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/remote_frame_client.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

ExecutionContext* RemoteDOMWindow::GetExecutionContext() const {
  return nullptr;
}

void RemoteDOMWindow::Trace(Visitor* visitor) const {
  DOMWindow::Trace(visitor);
}

RemoteDOMWindow::RemoteDOMWindow(RemoteFrame& frame) : DOMWindow(frame) {}

void RemoteDOMWindow::FrameDetached() {
  DisconnectFromFrame();
}

// SchedulePostMessage() was here. PostedMessage carried a
// SerializedScriptValue -- the structured clone of a script value -- and went
// with DOMWindow's declaration of it. postMessage() has no caller without a
// script engine.

// ForwardPostMessage() went with it: it sent the cross-process IPC.

}  // namespace blink
