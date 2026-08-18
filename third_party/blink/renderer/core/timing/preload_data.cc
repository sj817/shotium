// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/preload_data.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_cross_origin_mode.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/timing/cross_origin_mode_converter.h"
#include "third_party/blink/renderer/core/timing/dom_window_performance.h"
#include "third_party/blink/renderer/core/timing/window_performance.h"

namespace blink {

PreloadData::PreloadData(const KURL& url,
                         const String& as,
                         CrossOriginAttributeValue crossorigin,
                         bool earlyhint,
                         std::optional<base::TimeTicks> used_time)
    : url_(url),
      as_(as),
      crossorigin_(crossorigin),
      earlyhint_(earlyhint),
      used_time_(used_time) {}

V8CrossOriginMode PreloadData::crossorigin() const {
  return ToV8CrossOriginMode(crossorigin_);
}

void PreloadData::Trace(Visitor* visitor) const {
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
