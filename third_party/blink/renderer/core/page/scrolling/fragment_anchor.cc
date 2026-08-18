// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/page/scrolling/fragment_anchor.h"

#include "base/metrics/histogram_macros.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/mojom/scroll/scroll_into_view_params.mojom-blink.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/html/html_document.h"
#include "third_party/blink/renderer/core/page/scrolling/element_fragment_anchor.h"
#include "third_party/blink/renderer/core/scroll/scroll_alignment.h"
#include "third_party/blink/renderer/core/scroll/scroll_into_view_util.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

FragmentAnchor* FragmentAnchor::TryCreate(const KURL& url,
                                          LocalFrame& frame,
                                          bool should_scroll) {
  DCHECK(frame.GetDocument());

  // A TextFragmentAnchor was tried first, matching the URL's ":~:text=..."
  // directive against the rendered text and scrolling to it. That whole
  // feature is gone, so "#id" -- the element anchor, which is what a
  // screenshot of a local document actually uses -- is the only kind left.
  return ElementFragmentAnchor::TryCreate(url, frame, should_scroll);
}

void FragmentAnchor::ScrollElementIntoViewWithOptions(
    Element* element_to_scroll,
    ScrollIntoViewOptions* options) {
  if (element_to_scroll->GetLayoutObject()) {
    DCHECK(element_to_scroll->GetComputedStyle());
    mojom::blink::ScrollIntoViewParamsPtr params =
        scroll_into_view_util::CreateScrollIntoViewParams(
            *options, *element_to_scroll->GetComputedStyle());
    params->cross_origin_boundaries = false;
    element_to_scroll->ScrollIntoViewNoVisualUpdate(std::move(params));
  }
}

void FragmentAnchor::Trace(Visitor* visitor) const {
  visitor->Trace(frame_);
}

}  // namespace blink
