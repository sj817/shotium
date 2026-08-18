/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_ARRAY_BUFFER_ARRAY_BUFFER_CONTENTS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_ARRAY_BUFFER_ARRAY_BUFFER_CONTENTS_H_

#include <memory>

#include "base/containers/span.h"
#include "partition_alloc/partition_alloc_constants.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"
#include "third_party/blink/renderer/platform/wtf/cross_thread_copier.h"
#include "third_party/blink/renderer/platform/wtf/thread_safe_ref_counted.h"
#include "third_party/blink/renderer/platform/wtf/wtf.h"

namespace blink {

// Owns the bytes an ArrayBufferContents points at.
//
// This used to be V8's BackingStore, shared between Blink and V8 so that a
// JavaScript ArrayBuffer object and the Blink-side DOMArrayBuffer could refer
// to the same allocation. With V8 gone there is no second owner left, so the
// allocation is owned outright here: a block from
// `Partitions::ArrayBufferPartition()`, freed on destruction.
//
// Shared ownership between ArrayBufferContents copies is expressed with
// std::shared_ptr, exactly as it was with V8's BackingStore.
class CORE_EXPORT ArrayBufferBackingStore {
  USING_FAST_MALLOC(ArrayBufferBackingStore);

 public:
  ArrayBufferBackingStore(void* data,
                          size_t byte_length,
                          size_t max_byte_length,
                          bool is_shared)
      : data_(data),
        byte_length_(byte_length),
        max_byte_length_(max_byte_length),
        is_shared_(is_shared) {}

  ArrayBufferBackingStore(const ArrayBufferBackingStore&) = delete;
  ArrayBufferBackingStore& operator=(const ArrayBufferBackingStore&) = delete;

  ~ArrayBufferBackingStore();

  void* Data() const { return data_; }
  size_t ByteLength() const { return byte_length_; }
  size_t MaxByteLength() const { return max_byte_length_; }
  bool IsShared() const { return is_shared_; }

 private:
  void* data_ = nullptr;
  size_t byte_length_ = 0;
  size_t max_byte_length_ = 0;
  bool is_shared_ = false;
};

class CORE_EXPORT ArrayBufferContents {
  DISALLOW_NEW();

 public:
  enum InitializationPolicy { kZeroInitialize, kDontInitialize };

  enum SharingType {
    kNotShared,
    kShared,
  };

  // Behavior of a constructor on memory allocation failure.
  enum class AllocationFailureBehavior {
    // Construct an object for which `!IsValid()`.
    kInvalid,
    // Generate an OOM crash. The cause of the OOM (excessive size, mapping
    // failure, commit failure) can be derived from the crash stack, so this is
    // preferred to having custom logic in the caller to crash if `!IsValid()`.
    kCrash,
  };

  ArrayBufferContents() = default;
  ArrayBufferContents(size_t num_elements,
                      size_t element_byte_size,
                      SharingType is_shared,
                      InitializationPolicy policy,
                      AllocationFailureBehavior allocation_failure_behavior =
                          AllocationFailureBehavior::kInvalid);

  ArrayBufferContents(ArrayBufferContents&&) = default;

  ArrayBufferContents(const ArrayBufferContents&) = default;

  ~ArrayBufferContents();

  ArrayBufferContents& operator=(const ArrayBufferContents&) = default;
  ArrayBufferContents& operator=(ArrayBufferContents&&) = default;

  void Detach();

  // Resets the internal memory so that the ArrayBufferContents is empty.
  void Reset();

  void* Data() const {
    DCHECK(!IsShared());
    return DataMaybeShared();
  }
  void* DataShared() const {
    DCHECK(IsShared());
    return DataMaybeShared();
  }
  void* DataMaybeShared() const {
    return backing_store_ ? backing_store_->Data() : nullptr;
  }
  size_t DataLength() const {
    return backing_store_ ? backing_store_->ByteLength() : 0;
  }
  size_t MaxDataLength() const {
    return backing_store_ ? backing_store_->MaxByteLength() : 0;
  }
  bool IsShared() const { return backing_store_ && backing_store_->IsShared(); }
  bool IsValid() const { return backing_store_ && backing_store_->Data(); }
  base::span<uint8_t> ByteSpan() const {
    // SAFETY: `BackingStore` guarantees that `Data()` points to at least
    // `DataLength()` many bytes.
    return UNSAFE_BUFFERS(base::span(
        base::unchecked, static_cast<uint8_t*>(Data()), DataLength()));
  }
  base::span<uint8_t> ByteSpanShared() const {
    DCHECK(IsShared());
    return ByteSpanMaybeShared();
  }
  base::span<uint8_t> ByteSpanMaybeShared() const {
    // SAFETY: `BackingStore` guarantees that `Data()` points to at least
    // `DataLength()` many bytes.
    return UNSAFE_BUFFERS(base::span(base::unchecked,
                                     static_cast<uint8_t*>(DataMaybeShared()),
                                     DataLength()));
  }

  bool HasBackingStore() const { return backing_store_ != nullptr; }

  void Transfer(ArrayBufferContents& other);
  void ShareWith(ArrayBufferContents& other);
  void CopyTo(ArrayBufferContents& other);

  static void* AllocateMemoryOrNull(size_t, InitializationPolicy);
  static void FreeMemory(void*);

 private:
  template <partition_alloc::AllocFlags flags>
  static void* AllocateMemory(size_t, InitializationPolicy);

  std::shared_ptr<ArrayBufferBackingStore> backing_store_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_TYPED_ARRAYS_ARRAY_BUFFER_ARRAY_BUFFER_CONTENTS_H_
