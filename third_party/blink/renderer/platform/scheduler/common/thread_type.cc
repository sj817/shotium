// Copyright 2018 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/scheduler/public/thread_type.h"

#include "base/notreached.h"

namespace blink {

const char* GetNameForThreadType(ThreadType thread_type) {
  switch (thread_type) {
    case ThreadType::kMainThread:
      return "Main thread";
    case ThreadType::kTestThread:
      return "test thread";
    case ThreadType::kFontThread:
      return "Font thread";
    case ThreadType::kPreloadScannerThread:
      return "Preload scanner";
  }
  return nullptr;
}

}  // namespace blink
