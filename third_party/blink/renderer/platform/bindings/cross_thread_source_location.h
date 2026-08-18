// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_CROSS_THREAD_SOURCE_LOCATION_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_CROSS_THREAD_SOURCE_LOCATION_H_

#include "third_party/blink/renderer/platform/bindings/source_location.h"

namespace blink {

// Unlike the garbage-collected `SourceLocation`, a `CrossThreadSourceLocation`
// is safe to move across threads.
//
// This used to also carry a clone of SourceLocation's V8 stack trace (dropping
// its thread-hostile async-chain fields via `V8StackTrace::clone()`). V8 --
// and with it the stack trace -- is gone; only the URL/function/line/column
// fields remain, which were always plain, thread-safe value types.
class PLATFORM_EXPORT CrossThreadSourceLocation {
 public:
  CrossThreadSourceLocation() = default;

  explicit CrossThreadSourceLocation(const SourceLocation& location)
      : url_(location.Url()),
        function_(location.Function()),
        line_number_(location.LineNumber()),
        column_number_(location.ColumnNumber()),
        script_id_(location.ScriptId()) {}

  // TODO(crbug.com/462588571): Delete this if all the places that do
  // conversions never actually pass a null SourceLocation.
  static CrossThreadSourceLocation From(const SourceLocation* location);
  SourceLocation* ToSourceLocation() const {
    return MakeGarbageCollected<SourceLocation>(
        url_, function_, line_number_, column_number_, script_id_);
  }

 private:
  String url_;
  String function_;
  unsigned line_number_ = 0;
  unsigned column_number_ = 0;
  int script_id_ = 0;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_CROSS_THREAD_SOURCE_LOCATION_H_
