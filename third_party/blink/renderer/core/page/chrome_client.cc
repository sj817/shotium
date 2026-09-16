/*
 * Copyright (C) 2006, 2007, 2009, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2012, Samsung Electronics. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "third_party/blink/renderer/core/page/chrome_client.h"

#include <algorithm>

#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/core_initializer.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/element.h"
#include "third_party/blink/renderer/core/frame/frame_console.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/page/frame_tree.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "ui/display/screen_info.h"
#include "ui/gfx/geometry/rect.h"

namespace blink {

void ChromeClient::Trace(Visitor* visitor) const {}

void ChromeClient::InstallSupplements(LocalFrame& frame) {
  CoreInitializer::GetInstance().InstallSupplements(frame);
}

bool ChromeClient::CanOpenUIElementIfDuringPageDismissal(
    Frame& main_frame,
    UIElementType ui_element_type,
    const String& message) {
  for (Frame* frame = &main_frame; frame;
       frame = frame->Tree().TraverseNext()) {
    auto* local_frame = DynamicTo<LocalFrame>(frame);
    if (!local_frame)
      continue;
    Document::PageDismissalType dismissal =
        local_frame->GetDocument()->PageDismissalEventBeingDispatched();
    if (dismissal != Document::kNoDismissal) {
      return ShouldOpenUIElementDuringPageDismissal(
          *local_frame, ui_element_type, message, dismissal);
    }
  }
  return true;
}

Page* ChromeClient::CreateWindow(
    LocalFrame* frame,
    const FrameLoadRequest& r,
    const AtomicString& frame_name,
    const WebWindowFeatures& features,
    network::mojom::blink::WebSandboxFlags sandbox_flags,
    const SessionStorageNamespaceId& session_storage_namespace_id,
    bool& consumed_user_gesture) {
  if (!CanOpenUIElementIfDuringPageDismissal(
          frame->Tree().Top(), UIElementType::kPopup, g_empty_string)) {
    return nullptr;
  }

  return CreateWindowDelegate(frame, r, frame_name, features, sandbox_flags,
                              session_storage_namespace_id,
                              consumed_user_gesture);
}

bool ChromeClient::OpenBeforeUnloadConfirmPanel(const String& message,
                                                LocalFrame* frame,
                                                bool is_reload) {
  DCHECK(frame);
  return OpenBeforeUnloadConfirmPanelDelegate(frame, is_reload);
}
}  // namespace blink

