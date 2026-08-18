// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/bindings/source_location.h"

#include "third_party/blink/renderer/platform/bindings/script_forbidden_scope.h"
#include "third_party/blink/renderer/platform/instrumentation/tracing/traced_value.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"

namespace blink {

// static
SourceLocation* SourceLocation::CaptureWithFullStackTrace() {
  return MakeGarbageCollected<SourceLocation>(String(), String(), 0, 0, 0);
}

SourceLocation::SourceLocation(const String& url, int char_position)
    : url_(url),
      line_number_(0),
      column_number_(0),
      char_position_(char_position),
      script_id_(0) {}

SourceLocation::SourceLocation(const String& url,
                               int char_position,
                               unsigned line_number,
                               unsigned column_number)
    : url_(url),
      line_number_(line_number),
      column_number_(column_number),
      char_position_(char_position),
      script_id_(0) {}

SourceLocation::SourceLocation(const String& url,
                               const String& function,
                               unsigned line_number,
                               unsigned column_number,
                               int script_id)
    : url_(url),
      function_(function),
      line_number_(line_number),
      column_number_(column_number),
      char_position_(-1),
      script_id_(script_id) {}

SourceLocation::~SourceLocation() = default;

SourceLocation* SourceLocation::Clone() const {
  return MakeGarbageCollected<SourceLocation>(
      url_, function_, line_number_, column_number_, script_id_);
}

void SourceLocation::WriteIntoTrace(perfetto::TracedValue context) const {
  if (IsUnknown()) {
    return;
  }
  auto dict = std::move(context).WriteDictionary();
  dict.Add("functionName", function_);
  dict.Add("scriptId", String::Number(script_id_));
  dict.Add("url", url_);
  dict.Add("lineNumber", line_number_);
  dict.Add("columnNumber", column_number_);
}

void SourceLocation::ToTracedValue(TracedValue* value, const char* name) const {
  if (IsUnknown()) {
    return;
  }
  value->BeginDictionary(name);
  value->SetString("functionName", function_);
  value->SetInteger("scriptId", script_id_);
  value->SetString("url", url_);
  value->SetInteger("lineNumber", line_number_);
  value->SetInteger("columnNumber", column_number_);
  value->EndDictionary();
}

String SourceLocation::ToString() const {
  if (IsUnknown()) {
    return String();
  }
  StringBuilder builder;
  if (!function_.empty()) {
    builder.Append(function_);
    builder.Append(" (");
  }
  builder.Append(url_);
  builder.Append(':');
  builder.AppendNumber(line_number_);
  builder.Append(':');
  builder.AppendNumber(column_number_);
  if (!function_.empty()) {
    builder.Append(')');
  }
  return builder.ToString();
}

SourceLocation* CaptureSourceLocation(const String& url,
                                      unsigned line_number,
                                      unsigned column_number) {
  return MakeGarbageCollected<SourceLocation>(url, String(), line_number,
                                              column_number, 0);
}

SourceLocation* CaptureSourceLocation() {
  return MakeGarbageCollected<SourceLocation>(String(), String(), 0, 0, 0);
}

}  // namespace blink
