// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/heap/process_heap.h"

namespace blink {

// static
void ProcessHeap::Init() {
  // gin::InitializeCppgcFromV8Platform() handed cppgc the v8::Platform that
  // gin had already created, so the two shared one set of task runners. There
  // is no v8::Platform; ThreadState::AttachMainThread() creates the standalone
  // cppgc::Platform and the heap that sits on it, so there is nothing to do
  // process-wide first.
}

}  // namespace blink
