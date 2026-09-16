/*
 * Copyright (C) 2006 Eric Seidel (eric@webkit.org)
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_EMPTY_CLIENTS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_EMPTY_CLIENTS_H_

#include <memory>

#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/frame/local_frame_client.h"
#include "third_party/blink/renderer/core/page/chrome_client.h"
#include "ui/display/screen_info.h"
#include "ui/display/screen_infos.h"

namespace blink {

class AssociatedInterfaceProvider;

class CORE_EXPORT EmptyChromeClient : public ChromeClient {
 public:
  EmptyChromeClient() = default;
  ~EmptyChromeClient() override = default;

  const display::ScreenInfo& GetScreenInfo(LocalFrame&) const override {
    return empty_screen_infos_.current();
  }
  const display::ScreenInfos& GetScreenInfos(LocalFrame&) const override {
    return empty_screen_infos_;
  }
  const display::ScreenInfo& GetOriginalScreenInfo(LocalFrame&) const override {
    return empty_screen_infos_.current();
  }

 private:
  const display::ScreenInfos empty_screen_infos_{display::ScreenInfo()};
};

class CORE_EXPORT EmptyLocalFrameClient : public LocalFrameClient {
 public:
  EmptyLocalFrameClient() = default;
  EmptyLocalFrameClient(const EmptyLocalFrameClient&) = delete;
  EmptyLocalFrameClient& operator=(const EmptyLocalFrameClient&) = delete;
  ~EmptyLocalFrameClient() override = default;

  AssociatedInterfaceProvider* GetRemoteNavigationAssociatedInterfaces()
      override;

 protected:
  std::unique_ptr<AssociatedInterfaceProvider> associated_interface_provider_;
};

CORE_EXPORT ChromeClient& GetStaticEmptyChromeClientInstance();

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_LOADER_EMPTY_CLIENTS_H_
