/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011, 2012 Apple, Inc. All rights
 * reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CHROME_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CHROME_CLIENT_H_

#include <memory>

#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/time/time.h"
#include "cc/paint/draw_image.h"
#include "third_party/blink/public/common/dom_storage/session_storage_namespace_id.h"
#include "third_party/blink/public/common/input/web_input_event.h"
#include "third_party/blink/public/common/page/drag_operation.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/input/focus_type.mojom-blink-forward.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/html/forms/popup_menu.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"
#include "third_party/blink/renderer/core/loader/navigation_policy.h"
#include "third_party/blink/renderer/core/scroll/scroll_types.h"
#include "third_party/blink/renderer/core/style/computed_style_constants.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

// To avoid conflicts with the CreateWindow macro from the Windows SDK...
#undef CreateWindow

namespace display {
struct ScreenInfo;
struct ScreenInfos;
}  // namespace display

namespace ui {
class Cursor;
}

namespace blink {

class ColorChooser;
class ColorChooserClient;
class DateTimeChooser;
class DateTimeChooserClient;
class Element;
class Frame;
class HTMLFormControlElement;
class HTMLFormElement;
class HTMLInputElement;
class HTMLSelectElement;
class KeyboardEvent;
class LocalFrame;
class LocalFrameView;
class Node;
class Page;
class WebDragData;

struct DateTimeChooserParameters;
struct FrameLoadRequest;
struct WebWindowFeatures;

class CORE_EXPORT ChromeClient : public GarbageCollected<ChromeClient> {
 public:
  ChromeClient(const ChromeClient&) = delete;
  ChromeClient& operator=(const ChromeClient&) = delete;
  virtual ~ChromeClient() = default;


  // Converts the scalar value from window coordinates to viewport scale.
  virtual float WindowToViewportScalar(LocalFrame*,
                                       const float value) const = 0;

  virtual bool IsPopup() { return false; }

  virtual Element* GetPopupClientOwnerElement() { return nullptr; }

  virtual void ChromeDestroyed() = 0;

  virtual void SetWindowRect(const gfx::Rect&, LocalFrame&) = 0;
  virtual void MoveWindowTo(const gfx::Point&, LocalFrame&) = 0;
  virtual void ResizeWindowTo(const gfx::Size&, LocalFrame&) = 0;

  // For non-composited WebViews that exist to contribute to a "parent" WebView
  // painting. This informs the client of the area that needs to be redrawn.
  virtual void InvalidateContainer() = 0;

  // Converts the rect from local root coordinates (using the local root of the
  // given LocalFrameView) to screen coordinates. Performs the visual viewport
  // transform.
  virtual gfx::Rect LocalRootToScreenDIPs(const gfx::Rect&,
                                          const LocalFrameView*) const = 0;

  void ScheduleAnimation(const LocalFrameView* view,
                         cc::BeginMainFrameReason reason) {
    ScheduleAnimation(view, reason, base::TimeDelta(), /*urgent=*/false);
  }
  void ScheduleAnimation(const LocalFrameView* view,
                         base::TimeDelta delay = base::TimeDelta(),
                         bool urgent = false) {
    ScheduleAnimation(view, cc::BeginMainFrameReason::kOther, delay, urgent);
  }

  virtual void ScheduleAnimation(const LocalFrameView* view,
                                 cc::BeginMainFrameReason reason,
                                 base::TimeDelta delay,
                                 bool urgent) = 0;

  // This gives the rect of the top level window that the given LocalFrame is a
  // part of.
  virtual gfx::Rect RootWindowRect(LocalFrame&) = 0;

  // The LocalFrame pointer provides the ChromeClient with context about which
  // LocalFrame wants to create the new Page. Also, the newly created window
  // should not be shown to the user until the ChromeClient of the newly
  // created Page has its show method called.
  // The FrameLoadRequest parameter is only for ChromeClient to check if the
  // request could be fulfilled. The ChromeClient should not load the request.
  Page* CreateWindow(LocalFrame*,
                     const FrameLoadRequest&,
                     const AtomicString& frame_name,
                     const WebWindowFeatures&,
                     network::mojom::blink::WebSandboxFlags,
                     const SessionStorageNamespaceId&,
                     bool& consumed_user_gesture);

  virtual void AddMessageToConsole(LocalFrame*,
                                   mojom::ConsoleMessageSource,
                                   mojom::ConsoleMessageLevel,
                                   const String& message,
                                   unsigned line_number,
                                   const String& source_id,
                                   const String& stack_trace) = 0;

  virtual void CloseWindow() = 0;
  virtual bool TabsToLinks() = 0;

  virtual const display::ScreenInfo& GetScreenInfo(LocalFrame& frame) const = 0;
  virtual const display::ScreenInfos& GetScreenInfos(
      LocalFrame& frame) const = 0;

  virtual const display::ScreenInfo& GetOriginalScreenInfo(
      LocalFrame& frame) const = 0;

  virtual void SetCursor(const ui::Cursor&, LocalFrame* local_root) = 0;
  virtual void SetCursorOverridden(bool) = 0;

  virtual void ContentsSizeChanged(LocalFrame*, const gfx::Size&) const = 0;
  virtual void OutermostMainFrameScrollOffsetChanged() const = 0;

  virtual ColorChooser* OpenColorChooser(LocalFrame*,
                                         ColorChooserClient*,
                                         const Color&) = 0;

  // This function is used for:
  //  - Mandatory date/time choosers if InputMultipleFieldsUI flag is not set
  //  - Date/time choosers for types for which
  //    LayoutTheme::SupportsCalendarPicker returns true, if
  //    InputMultipleFieldsUI flag is set
  //  - <datalist> UI for date/time input types regardless of
  //    InputMultipleFieldsUI flag
  // |LocalFrame| should not be null.
  virtual DateTimeChooser* OpenDateTimeChooser(
      LocalFrame*,
      DateTimeChooserClient*,
      const DateTimeChooserParameters&) = 0;
  virtual void OpenTextDataListChooser(HTMLInputElement&) = 0;

  // Checks if there is an opened popup, called by LayoutMenuList::showPopUp().
  virtual bool HasOpenedPopup() const = 0;
  virtual PopupMenu* OpenPopupMenu(LocalFrame&, HTMLSelectElement&) = 0;

  // Allow overriding whether external popup menus are used.
  virtual bool UseExternalPopupMenus() const { return false; }

  enum class UIElementType {
    kPopup = 0,
  };
  virtual bool ShouldOpenUIElementDuringPageDismissal(
      LocalFrame&,
      UIElementType,
      const String&,
      Document::PageDismissalType) const {
    return false;
  }

  virtual bool IsIsolatedSVGChromeClient() const { return false; }

  virtual void InstallSupplements(LocalFrame&);

  virtual void RequestDecode(LocalFrame*,
                             const cc::DrawImage& image,
                             base::OnceCallback<void(bool)> callback,
                             bool speculative) {
    std::move(callback).Run(false);
  }

  virtual void Trace(Visitor*) const;

 protected:
  ChromeClient() = default;

  virtual Page* CreateWindowDelegate(LocalFrame*,
                                     const FrameLoadRequest&,
                                     const AtomicString& frame_name,
                                     const WebWindowFeatures&,
                                     network::mojom::blink::WebSandboxFlags,
                                     const SessionStorageNamespaceId&,
                                     bool& consumed_user_gesture) = 0;

 private:
  bool CanOpenUIElementIfDuringPageDismissal(Frame& main_frame,
                                             UIElementType,
                                             const String& message);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CHROME_CLIENT_H_
