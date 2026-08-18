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

#include "third_party/blink/renderer/core/typed_arrays/array_buffer/array_buffer_contents.h"

#include <cstring>
#include <limits>

#include "base/bits.h"
#include "base/numerics/checked_math.h"
#include "partition_alloc/oom.h"
#include "partition_alloc/partition_alloc.h"
#include "third_party/blink/renderer/platform/instrumentation/instance_counters.h"
#include "third_party/blink/renderer/platform/wtf/allocator/partitions.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"

namespace blink {

ArrayBufferBackingStore::~ArrayBufferBackingStore() {
  if (data_) {
    ArrayBufferContents::FreeMemory(data_);
  }
}

ArrayBufferContents::ArrayBufferContents(
    size_t num_elements,
    size_t element_byte_size,
    SharingType is_shared,
    ArrayBufferContents::InitializationPolicy policy,
    AllocationFailureBehavior allocation_failure_behavior) {
  auto checked_length =
      base::CheckedNumeric<size_t>(num_elements) * element_byte_size;
  if (!checked_length.IsValid()) {
    // The requested size is too big.
    if (allocation_failure_behavior == AllocationFailureBehavior::kCrash) {
      OOM_CRASH(std::numeric_limits<size_t>::max());
    }
    return;
  }
  size_t length = checked_length.ValueOrDie();

  void* data =
      (allocation_failure_behavior == AllocationFailureBehavior::kCrash)
          ? AllocateMemory<partition_alloc::AllocFlags::kNone>(length, policy)
          : AllocateMemoryOrNull(length, policy);

  if (data) {
    backing_store_ = std::make_shared<ArrayBufferBackingStore>(
        data, length, length, is_shared == kShared);
  }

  if (allocation_failure_behavior == AllocationFailureBehavior::kCrash &&
      !IsValid()) {
    // All code paths that fail to allocate memory should crash. This is added
    // as an extra precaution.
    OOM_CRASH(length);
  }
}

ArrayBufferContents::~ArrayBufferContents() = default;

void ArrayBufferContents::Detach() {
  backing_store_.reset();
}

void ArrayBufferContents::Reset() {
  backing_store_.reset();
}

void ArrayBufferContents::Transfer(ArrayBufferContents& other) {
  DCHECK(!IsShared());
  DCHECK(!other.Data());
  other.backing_store_ = std::move(backing_store_);
}

void ArrayBufferContents::ShareWith(ArrayBufferContents& other) {
  DCHECK(IsShared());
  DCHECK(!other.Data());
  other.backing_store_ = backing_store_;
}

void ArrayBufferContents::CopyTo(ArrayBufferContents& other) {
  other = ArrayBufferContents(
      DataLength(), 1, IsShared() ? kShared : kNotShared, kDontInitialize);
  if (!IsValid() || !other.IsValid())
    return;
  other.ByteSpan().copy_from(ByteSpan());
}

template <partition_alloc::AllocFlags flags>
void* ArrayBufferContents::AllocateMemory(size_t size,
                                          InitializationPolicy policy) {
  // The array buffer contents are sometimes expected to be 16-byte aligned in
  // order to get the best optimization of SSE, especially in case of audio and
  // video buffers.  Hence, align the given size up to 16-byte boundary.
  // Technically speaking, 16-byte aligned size doesn't mean 16-byte aligned
  // address, but this heuristics works with the current implementation of
  // PartitionAlloc (and PartitionAlloc doesn't support a better way for now).
  //
  // `partition_alloc::internal::kAlignment` is a compile-time constant.
  if (partition_alloc::internal::kAlignment < 16) {
    size_t aligned_size = base::bits::AlignUp(size, size_t{16});
    if (size == 0) {
      aligned_size = 16;
    }
    if (aligned_size >= size) {  // Only when no overflow
      size = aligned_size;
    }
  }

  constexpr auto new_flags = flags;
  void* data;
  if (policy == kZeroInitialize) {
    data = Partitions::ArrayBufferPartition()
               ->Alloc<new_flags | partition_alloc::AllocFlags::kZeroFill>(
                   size, WTF_HEAP_PROFILER_TYPE_NAME(ArrayBufferContents));
  } else {
    data = Partitions::ArrayBufferPartition()->Alloc<new_flags>(
        size, WTF_HEAP_PROFILER_TYPE_NAME(ArrayBufferContents));
  }

  if (partition_alloc::internal::kAlignment < 16) {
    char* ptr = reinterpret_cast<char*>(data);
    DCHECK_EQ(base::bits::AlignUp(ptr, 16), ptr)
        << "Pointer " << ptr << " not 16B aligned for size " << size;
  }
  InstanceCounters::IncrementCounter(
      InstanceCounters::kArrayBufferContentsCounter);
  return data;
}

void* ArrayBufferContents::AllocateMemoryOrNull(size_t size,
                                                InitializationPolicy policy) {
  return AllocateMemory<partition_alloc::AllocFlags::kReturnNull>(size, policy);
}

void ArrayBufferContents::FreeMemory(void* data) {
  InstanceCounters::DecrementCounter(
      InstanceCounters::kArrayBufferContentsCounter);
  Partitions::ArrayBufferPartition()->Free(data);
}

}  // namespace blink
