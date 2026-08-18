// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/fileapi/url_file_api.h"

#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fileapi/public_url_manager.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"

namespace blink {

// static
void URLFileAPI::revokeObjectURL(ExecutionContext* execution_context,
                                 const String& url_string) {
  DCHECK(execution_context);

  KURL url(NullUrl(), url_string);
  execution_context->RemoveURLFromMemoryCache(url);
  execution_context->GetPublicURLManager().Revoke(url);
}

}  // namespace blink
