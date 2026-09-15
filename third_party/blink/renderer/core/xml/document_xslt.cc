// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/xml/document_xslt.h"

#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/processing_instruction.h"
#include "third_party/blink/renderer/core/xml/xslt_processor.h"

namespace blink {

DocumentXSLT::DocumentXSLT(Document& document)
    : Supplement<Document>(document) {}

void DocumentXSLT::ApplyXSLTransform(Document&, ProcessingInstruction*) {}

ProcessingInstruction* DocumentXSLT::FindXSLStyleSheet(Document&) {
  return nullptr;
}

bool DocumentXSLT::ProcessingInstructionInsertedIntoDocument(
    Document&,
    ProcessingInstruction*) {
  return false;
}

bool DocumentXSLT::ProcessingInstructionRemovedFromDocument(
    Document&,
    ProcessingInstruction*) {
  return false;
}

bool DocumentXSLT::SheetLoaded(Document&, ProcessingInstruction*) {
  return false;
}

// static
const char DocumentXSLT::kSupplementName[] = "DocumentXSLT";

bool DocumentXSLT::HasTransformSourceDocument(Document& document) {
  return Supplement<Document>::From<DocumentXSLT>(document);
}

void DocumentXSLT::SetHasTransformSource(Document& document) {
  DCHECK(!HasTransformSourceDocument(document));
  Supplement<Document>::ProvideTo(document,
                                  MakeGarbageCollected<DocumentXSLT>(document));
}

void DocumentXSLT::Trace(Visitor* visitor) const {
  Supplement<Document>::Trace(visitor);
}

}  // namespace blink
