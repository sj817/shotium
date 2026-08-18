// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/probe/async_task_context.h"

#include "base/trace_event/trace_id_helper.h"
#include "base/trace_event/typed_macros.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"

namespace blink {
namespace probe {

AsyncTaskContext::~AsyncTaskContext() {
  Cancel();
}

void AsyncTaskContext::Schedule(ExecutionContext* context,
                                const StringView& name,
                                StackOptions stack_options) {
  // TODO(crbug.com/1275875): Verify that this context was not already
  // scheduled or has already been canceled. Currently we don't have enough
  // confidence that such a CHECK wouldn't break blink.
  TRACE_EVENT("blink", "AsyncTask Scheduled",
              perfetto::Flow::FromPointer(this));

  if (!context)
    return;

  // StackOptions::kScan asked ScriptInitiationMonitor to walk the JS stack
  // and attribute this task to the script that scheduled it. There is no JS
  // stack. The option is still accepted so callers keep documenting intent.
}

void AsyncTaskContext::Cancel() {}

void* AsyncTaskContext::Id() const {
  // Blink uses odd ids for network requests and even ids for everything else;
  // shifting makes all of them even.
  return reinterpret_cast<void*>(reinterpret_cast<intptr_t>(this) << 1);
}

}  // namespace probe
}  // namespace blink
