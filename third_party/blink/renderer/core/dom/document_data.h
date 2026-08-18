// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_DATA_H_

#include <optional>

#include "base/time/time.h"
#include "third_party/blink/public/mojom/permissions/permission.mojom-blink.h"
#include "third_party/blink/renderer/platform/mojo/heap_mojo_remote.h"

namespace blink {

// The purpose of blink::DocumentData is to reduce the size of document.h.
// Data members which require huge headers should be stored in
// blink::DocumentData instead of blink::Document.
//
// Ownership: A Document has a strong reference to a single DocumentData.
//   Other instances should not have strong references to the DocumentData.
// Lifetime: A DocumentData instance is created on a Document creation, and
//   is never destructed before the Document.
class DocumentData final : public GarbageCollected<DocumentData> {
 public:
  explicit DocumentData(ExecutionContext* context)
      : permission_service_(context) {}

  void Trace(Visitor* visitor) const { visitor->Trace(permission_service_); }

 private:
  // Mojo remote used to determine if the document has permission to access
  // storage or not.
  HeapMojoRemote<mojom::blink::PermissionService> permission_service_;

  // The number of immediate child frames created within this document so far.
  // This count doesn't include this document's frame nor descendant frames.
  int immediate_child_frame_creation_count_ = 0;

  // LCPP's LCP ElementLocator was matched against a tag against html
  // during preload scanning.
  bool lcpp_encountered_lcp_in_html = false;

  // Measures SVGImage performance per document.
  int svg_image_processed_count_ = 0;
  base::TimeDelta accumulated_svg_image_elapsed_time_;

  // Start time of XML parser. Used for benchmarking XML parsing performance.
  std::optional<base::TimeTicks> xml_parser_start_time_;
  // Whether the Rust XML parser is used.
  bool using_rust_xml_parser_ = false;

  friend class Document;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_DOCUMENT_DATA_H_
