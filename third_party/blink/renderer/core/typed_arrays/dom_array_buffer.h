// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_DOM_ARRAY_BUFFER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_DOM_ARRAY_BUFFER_H_

#include <algorithm>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "partition_alloc/oom.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/typed_arrays/array_buffer/array_buffer_contents.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer_base.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

class CORE_EXPORT DOMArrayBuffer : public DOMArrayBufferBase {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static DOMArrayBuffer* Create(ArrayBufferContents contents) {
    return MakeGarbageCollected<DOMArrayBuffer>(std::move(contents));
  }
  static DOMArrayBuffer* Create(size_t num_elements, size_t element_byte_size) {
    ArrayBufferContents contents(
        num_elements, element_byte_size, ArrayBufferContents::kNotShared,
        ArrayBufferContents::kZeroInitialize,
        ArrayBufferContents::AllocationFailureBehavior::kCrash);
    CHECK(contents.IsValid());
    return Create(std::move(contents));
  }
  static DOMArrayBuffer* Create(base::span<const uint8_t> source) {
    ArrayBufferContents contents(
        source.size(), 1, ArrayBufferContents::kNotShared,
        ArrayBufferContents::kDontInitialize,
        ArrayBufferContents::AllocationFailureBehavior::kCrash);
    CHECK(contents.IsValid());
    contents.ByteSpan().copy_from(source);
    return Create(std::move(contents));
  }

  static DOMArrayBuffer* Create(scoped_refptr<SharedBuffer>);
  static DOMArrayBuffer* Create(const Vector<base::span<const uint8_t>>&);

  static DOMArrayBuffer* CreateOrNull(size_t num_elements,
                                      size_t element_byte_size);
  static DOMArrayBuffer* CreateOrNull(base::span<const uint8_t> source);

  // For use by DOMTypedArray.
  static DOMArrayBuffer* CreateUninitialized(size_t num_elements,
                                             size_t element_byte_size);
  // Only for use by XMLHttpRequest::responseArrayBuffer,
  // Internals::serializeObject, and
  // FetchDataLoaderAsArrayBuffer::OnStateChange.
  static DOMArrayBuffer* CreateUninitializedOrNull(size_t num_elements,
                                                   size_t element_byte_size);

  explicit DOMArrayBuffer(ArrayBufferContents contents)
      : DOMArrayBufferBase(std::move(contents)) {}

  DOMArrayBuffer* Slice(size_t begin, size_t end) const;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_DOM_ARRAY_BUFFER_H_
