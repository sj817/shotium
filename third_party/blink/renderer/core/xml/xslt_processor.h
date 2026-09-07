/*
 * This file is part of the XSL implementation.
 *
 * Copyright (C) 2004, 2007, 2008 Apple, Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_XML_XSLT_PROCESSOR_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_XML_XSLT_PROCESSOR_H_

#include <libxml/parserInternals.h>
#include <libxslt/documents.h>

#include "base/types/pass_key.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/inspector/console_message.h"
#include "third_party/blink/renderer/core/xml/xsl_style_sheet.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace blink {

class LocalFrame;
class Document;

class XSLTProcessor final : public GarbageCollected<XSLTProcessor> {
 public:
  using PassKey = base::PassKey<XSLTProcessor>;
  static XSLTProcessor* Create(Document& document) {
    return MakeGarbageCollected<XSLTProcessor>(XSLTProcessor::PassKey(), document);
  }
  XSLTProcessor(PassKey, Document&);
  ~XSLTProcessor();

  static void ReportXSLTDisabled(Document& document);

  static bool IsXSLTEnabled(const ExecutionContext* context);

  void SetXSLStyleSheet(XSLStyleSheet* style_sheet) {
    stylesheet_ = style_sheet;
  }
  bool TransformToString(Node* source,
                         String& result_mime_type,
                         String& result_string,
                         String& result_encoding);
  Document* CreateDocumentFromSource(const String& source,
                                     const String& source_encoding,
                                     const String& source_mime_type,
                                     Node* source_node,
                                     LocalFrame*);

  static void ParseErrorFunc(void* user_data, const xmlError*);
  static void GenericErrorFunc(void* user_data, const char* msg, ...);

  // Only for libXSLT callbacks
  XSLStyleSheet* XslStylesheet() const { return stylesheet_.Get(); }

  void Trace(Visitor*) const;

 private:
  Member<XSLStyleSheet> stylesheet_;
  Member<Document> document_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_XML_XSLT_PROCESSOR_H_
