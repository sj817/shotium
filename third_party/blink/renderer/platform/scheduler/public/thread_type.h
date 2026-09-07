// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_THREAD_TYPE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_THREAD_TYPE_H_

#include "third_party/blink/public/platform/web_common.h"

namespace blink {

enum class ThreadType {
  kMainThread = 0,
  // 7 was kAudioWorkletThread, which was deleted (crbug.com/1051992)
  // 10 was kWebAudioThread, which was deleted (crbug.com/965093)
  // 11 was kScriptStreamerThread, which was deleted
  kTestThread = 15,
  kFontThread = 21,
  kPreloadScannerThread = 22,

  kMaxValue = kPreloadScannerThread,
};

BLINK_PLATFORM_EXPORT const char* GetNameForThreadType(ThreadType);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_SCHEDULER_PUBLIC_THREAD_TYPE_H_
