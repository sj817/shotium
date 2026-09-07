/*
 * Copyright (C) 2004, 2006, 2007 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/canvas/html_canvas_element.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/css/style_change_reason.h"
#include "third_party/blink/renderer/core/layout/layout_object_inlines.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {
HTMLCanvasElement::HTMLCanvasElement(Document& document)
    : HTMLElement(html_names::kCanvasTag, document) {
  UseCounter::Count(GetDocument(), WebFeature::kHTMLCanvasElement);
  SetHasCustomStyleCallbacks();
}

void HTMLCanvasElement::AttributeChanged(
    const AttributeModificationParams& params) {
  HTMLElement::AttributeChanged(params);

  if (RuntimeEnabledFeatures::CanvasDrawElementEnabled(GetExecutionContext()) &&
      params.name == html_names::kLayoutsubtreeAttr) {
    bool had_layoutsubtree = !params.old_value.IsNull();
    bool has_layoutsubtree = !params.new_value.IsNull();
    if (had_layoutsubtree != has_layoutsubtree) {
      InvalidateLayoutSubtree();
    }
  }
}

void HTMLCanvasElement::InvalidateLayoutSubtree() {
  SetNeedsStyleRecalc(
      kSubtreeStyleChange,
      StyleChangeReasonForTracing::Create(style_change_reason::kAttribute));
  SetForceReattachLayoutTree();
  if (auto* object = GetLayoutObject()) {
    object->SetNeedsLayout(layout_invalidation_reason::kAttributeChanged);
  }
}

bool HTMLCanvasElement::layoutSubtree() const {
  return FastHasAttribute(html_names::kLayoutsubtreeAttr);
}

bool HTMLCanvasElement::IsPresentationAttribute(
    const QualifiedName& name) const {
  if (name == html_names::kWidthAttr || name == html_names::kHeightAttr)
    return true;
  return HTMLElement::IsPresentationAttribute(name);
}

void HTMLCanvasElement::CollectStyleForPresentationAttribute(
    const QualifiedName& name,
    const AtomicString& value,
    HeapVector<CSSPropertyValue, 8>& style) {
  if (name == html_names::kWidthAttr) {
    const AtomicString& height = FastGetAttribute(html_names::kHeightAttr);
    if (!height.IsNull())
      ApplyIntegerAspectRatioToStyle(value, height, style);
  } else if (name == html_names::kHeightAttr) {
    const AtomicString& width = FastGetAttribute(html_names::kWidthAttr);
    if (!width.IsNull())
      ApplyIntegerAspectRatioToStyle(width, value, style);
  } else {
    HTMLElement::CollectStyleForPresentationAttribute(name, value, style);
  }
}

bool HTMLCanvasElement::CanStartSelection() const {
  if (!layoutSubtree()) {
    return false;
  }
  return HTMLElement::CanStartSelection();
}

void HTMLCanvasElement::ChildrenChanged(const ChildrenChange& change) {
  HTMLElement::ChildrenChanged(change);
  if (hasChildren()) {
    UseCounter::Count(GetDocument(), WebFeature::kCanvasFallbackContent);
    if (firstElementChild())
      UseCounter::Count(GetDocument(), WebFeature::kCanvasFallbackElementContent);
  }
}

}  // namespace blink
