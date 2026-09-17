// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/xml/xsl_style_sheet.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/node.h"

namespace blink {

XSLStyleSheet::XSLStyleSheet(XSLStyleSheet* parent_style_sheet,
                             const String& original_url,
                             const KURL& final_url)
    : owner_node_(nullptr),
      original_url_(original_url),
      final_url_(final_url),
      is_disabled_(false),
      parent_style_sheet_(parent_style_sheet) {}

XSLStyleSheet::XSLStyleSheet(Node* parent_node,
                             const String& original_url,
                             const KURL& final_url,
                             bool /*embedded*/)
    : owner_node_(parent_node),
      original_url_(original_url),
      final_url_(final_url),
      is_disabled_(false),
      parent_style_sheet_(nullptr) {}

XSLStyleSheet::~XSLStyleSheet() = default;

bool XSLStyleSheet::ParseString(const String&) {
  return false;
}

void XSLStyleSheet::CheckLoaded() {}

Document* XSLStyleSheet::OwnerDocument() {
  for (XSLStyleSheet* style_sheet = this; style_sheet;
       style_sheet = style_sheet->parentStyleSheet()) {
    if (style_sheet->ownerNode()) {
      return &style_sheet->ownerNode()->GetDocument();
    }
  }
  return nullptr;
}

void XSLStyleSheet::Trace(Visitor* visitor) const {
  visitor->Trace(owner_node_);
  visitor->Trace(children_);
  visitor->Trace(parent_style_sheet_);
  StyleSheet::Trace(visitor);
}

}  // namespace blink
