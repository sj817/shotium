// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"

#include <algorithm>

#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/wtf/shared_buffer.h"

namespace blink {

DOMArrayBuffer* DOMArrayBuffer::Create(
    scoped_refptr<SharedBuffer> shared_buffer) {
  ArrayBufferContents contents(
      shared_buffer->size(), 1, ArrayBufferContents::kNotShared,
      ArrayBufferContents::kDontInitialize,
      ArrayBufferContents::AllocationFailureBehavior::kCrash);
  CHECK(contents.IsValid());

  auto contents_bytes = contents.ByteSpan();
  for (const auto& span : *shared_buffer) {
    contents_bytes.take_first(span.size()).copy_from(base::as_bytes(span));
  }
  return Create(std::move(contents));
}

DOMArrayBuffer* DOMArrayBuffer::Create(
    const Vector<base::span<const uint8_t>>& data) {
  size_t size = 0;
  for (const auto& span : data) {
    size += span.size();
  }
  ArrayBufferContents contents(
      size, 1, ArrayBufferContents::kNotShared,
      ArrayBufferContents::kDontInitialize,
      ArrayBufferContents::AllocationFailureBehavior::kCrash);
  CHECK(contents.IsValid());

  auto contents_bytes = contents.ByteSpan();
  for (const auto& span : data) {
    contents_bytes.take_first(span.size()).copy_from(span);
  }
  return Create(std::move(contents));
}

DOMArrayBuffer* DOMArrayBuffer::CreateOrNull(size_t num_elements,
                                             size_t element_byte_size) {
  ArrayBufferContents contents(num_elements, element_byte_size,
                               ArrayBufferContents::kNotShared,
                               ArrayBufferContents::kZeroInitialize);
  if (!contents.Data()) {
    return nullptr;
  }
  return Create(std::move(contents));
}

DOMArrayBuffer* DOMArrayBuffer::CreateOrNull(base::span<const uint8_t> source) {
  DOMArrayBuffer* buffer = CreateUninitializedOrNull(source.size(), 1);
  if (!buffer) {
    return nullptr;
  }

  buffer->ByteSpan().copy_from(source);
  return buffer;
}

DOMArrayBuffer* DOMArrayBuffer::CreateUninitialized(size_t num_elements,
                                                    size_t element_byte_size) {
  ArrayBufferContents contents(
      num_elements, element_byte_size, ArrayBufferContents::kNotShared,
      ArrayBufferContents::kDontInitialize,
      ArrayBufferContents::AllocationFailureBehavior::kCrash);
  CHECK(contents.IsValid());
  return Create(std::move(contents));
}

DOMArrayBuffer* DOMArrayBuffer::CreateUninitializedOrNull(
    size_t num_elements,
    size_t element_byte_size) {
  ArrayBufferContents contents(num_elements, element_byte_size,
                               ArrayBufferContents::kNotShared,
                               ArrayBufferContents::kDontInitialize);
  if (!contents.Data()) {
    return nullptr;
  }
  return Create(std::move(contents));
}

DOMArrayBuffer* DOMArrayBuffer::Slice(size_t begin, size_t end) const {
  begin = std::min(begin, ByteLength());
  end = std::min(end, ByteLength());
  size_t size = begin <= end ? end - begin : 0;
  return Create(ByteSpan().subspan(begin, size));
}

}  // namespace blink
