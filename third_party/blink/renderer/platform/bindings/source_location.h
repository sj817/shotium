// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SOURCE_LOCATION_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SOURCE_LOCATION_H_

#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_copier.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "third_party/perfetto/include/perfetto/tracing/traced_value_forward.h"

namespace blink {

class TracedValue;

// A location in source code: a URL plus a line/column (or character offset)
// within it, and optionally a function name and a script id.
//
// This used to also carry a full V8 stack trace (v8_inspector::V8StackTrace),
// captured from the currently-running JS call stack via ThreadDebugger, and
// exposed it for DevTools through StackTrace()/BuildInspectorObject(). V8 is
// gone, so there is no JS call stack left to capture. This class keeps the
// URL/line/column/function path -- which callers populate directly, not by
// capturing a stack -- and drops the stack-capture machinery along with the
// APIs that only made sense for a multi-frame trace.
class PLATFORM_EXPORT SourceLocation final
    : public GarbageCollected<SourceLocation> {
 public:
  // Degraded equivalent of the old "capture the full JS stack trace" entry
  // point: with no JS stack to walk, this always returns an unknown
  // location. Kept under its original name/signature because its only
  // callers just check IsUnknown() / call ToString() on the result.
  static SourceLocation* CaptureWithFullStackTrace();

  SourceLocation(const String& url, int char_position);

  SourceLocation(const String& url,
                 int char_position,
                 unsigned line_number,
                 unsigned column_number);

  SourceLocation(const String& url,
                 const String& function,
                 unsigned line_number,
                 unsigned column_number,
                 int script_id = 0);
  void Trace(Visitor*) const {}
  ~SourceLocation();

  bool IsUnknown() const {
    return url_.IsNull() && !script_id_ && !line_number_;
  }
  const String& Url() const { return url_; }
  const String& Function() const { return function_; }
  unsigned LineNumber() const { return line_number_; }
  unsigned ColumnNumber() const { return column_number_; }
  int CharPosition() const { return char_position_; }
  int ScriptId() const { return script_id_; }

  SourceLocation* Clone() const;

  // TODO(altimin): Remove TracedValue version.
  void WriteIntoTrace(perfetto::TracedValue context) const;

  // No-op when the location is unknown.
  // TODO(altimin): Replace all usages of `ToTracedValue` with
  // `WriteIntoTrace` and remove this method.
  void ToTracedValue(TracedValue*, const char* name) const;

  // Could be a null string when the location is unknown.
  String ToString() const;

 private:
  String url_;
  String function_;
  unsigned line_number_;
  unsigned column_number_;
  int char_position_;
  int script_id_;
};

// Zero lineNumber and columnNumber mean unknown.
PLATFORM_EXPORT SourceLocation* CaptureSourceLocation(const String& url,
                                                      unsigned line_number,
                                                      unsigned column_number);

// Returns an unknown SourceLocation (IsUnknown() == true). There is no
// current JS stack left to capture a location from; this is kept, under its
// original no-argument signature, purely so callers that ask for "the
// current source location" with nothing else to go on keep compiling.
PLATFORM_EXPORT SourceLocation* CaptureSourceLocation();

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SOURCE_LOCATION_H_
