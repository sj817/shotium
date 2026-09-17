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

#include "base/time/time.h"
#include "cc/metrics/begin_main_frame_metrics.h"
#include "third_party/blink/public/mojom/devtools/console_message.mojom-blink-forward.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"

namespace display {
struct ScreenInfo;
struct ScreenInfos;
}  // namespace display

namespace blink {

class LocalFrame;
class LocalFrameView;

class CORE_EXPORT ChromeClient : public GarbageCollected<ChromeClient> {
 public:
  ChromeClient(const ChromeClient&) = delete;
  ChromeClient& operator=(const ChromeClient&) = delete;
  virtual ~ChromeClient() = default;

  float WindowToViewportScalar(LocalFrame*, const float value) const {
    return value;
  }

  virtual void ChromeDestroyed() {}

  // For non-composited WebViews that exist to contribute to a "parent" WebView
  // painting. This informs the client of the area that needs to be redrawn.
  virtual void InvalidateContainer() {}

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
                                 bool urgent) {}

  virtual void AddMessageToConsole(LocalFrame*,
                                   mojom::ConsoleMessageSource,
                                   mojom::ConsoleMessageLevel,
                                   const String& message,
                                   unsigned line_number,
                                   const String& source_id,
                                   const String& stack_trace) {}

  virtual const display::ScreenInfo& GetScreenInfo(LocalFrame& frame) const = 0;
  virtual const display::ScreenInfos& GetScreenInfos(
      LocalFrame& frame) const = 0;

  virtual const display::ScreenInfo& GetOriginalScreenInfo(
      LocalFrame& frame) const = 0;

  virtual bool IsIsolatedSVGChromeClient() const { return false; }

  virtual void InstallSupplements(LocalFrame&);

  virtual void Trace(Visitor*) const;

 protected:
  ChromeClient() = default;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_CHROME_CLIENT_H_
