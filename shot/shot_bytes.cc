// Copyright 2026 The Shot Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_bytes.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/compiler_specific.h"
#include "partition_alloc/page_allocator.h"
#include "partition_alloc/page_allocator_constants.h"

namespace shot {

namespace {

// How much is committed at a time as a writer grows. Large enough that a
// 60 MB image commits in a few dozen steps, small enough that the waste past
// the last byte -- freed at Finish() anyway -- never matters.
constexpr size_t kCommitStep = 1 << 20;

// How much a file-mode writer collects before writing it out. A 67 MB PNG is
// then some 270 writes rather than 25,000, for a quarter megabyte held.
constexpr size_t kFileStageBytes = 256 << 10;

size_t RoundUp(size_t bytes, size_t to) {
  return (bytes + to - 1) / to * to;
}

// SAFETY: `pages` is a reservation of at least `offset + length` bytes, of
// which the first `committed` are accessible; callers keep to those.
base::span<uint8_t> PageSpan(uintptr_t pages, size_t offset, size_t length) {
  return UNSAFE_BUFFERS(
      base::span(reinterpret_cast<uint8_t*>(pages) + offset, length));
}

}  // namespace

Bytes::Bytes() = default;

Bytes::Bytes(Bytes&& other) noexcept
    : heap_(std::move(other.heap_)),
      pages_(std::exchange(other.pages_, 0)),
      reserved_(std::exchange(other.reserved_, 0)),
      size_(std::exchange(other.size_, 0)) {}

Bytes& Bytes::operator=(Bytes&& other) noexcept {
  if (this != &other) {
    Release();
    heap_ = std::move(other.heap_);
    pages_ = std::exchange(other.pages_, 0);
    reserved_ = std::exchange(other.reserved_, 0);
    size_ = std::exchange(other.size_, 0);
  }
  return *this;
}

Bytes::~Bytes() {
  Release();
}

void Bytes::Release() {
  if (pages_) {
    partition_alloc::FreePages(pages_, reserved_);
    pages_ = 0;
    reserved_ = 0;
  }
  heap_.clear();
  size_ = 0;
}

// static
Bytes Bytes::Copy(base::span<const uint8_t> bytes) {
  return FromVector(std::vector<uint8_t>(bytes.begin(), bytes.end()));
}

// static
Bytes Bytes::FromVector(std::vector<uint8_t> bytes) {
  Bytes out;
  out.size_ = bytes.size();
  out.heap_ = std::move(bytes);
  return out;
}

base::span<const uint8_t> Bytes::span() const {
  if (pages_) {
    return PageSpan(pages_, 0, size_);
  }
  return base::span(heap_).first(size_);
}

Bytes::Writer::Writer(size_t capacity) {
  if (capacity == 0) {
    return;
  }
  // Small screenshots usually encode to a few kilobytes. Reserving virtual
  // address space and committing a megabyte for each one costs more than a
  // growable buffer; keep the page-backed writer for large outputs.
  if (capacity <= (16u << 20)) {
    heap_.reserve(std::min(capacity, size_t{64} << 10));
    return;
  }
  const size_t granularity =
      partition_alloc::internal::PageAllocationGranularity();
  reserved_ = RoundUp(capacity, granularity);
  pages_ = partition_alloc::AllocPages(
      reserved_, granularity,
      partition_alloc::PageAccessibilityConfiguration(
          partition_alloc::PageAccessibilityConfiguration::kInaccessible),
      partition_alloc::PageTag::kChromium);
  if (!pages_) {
    reserved_ = 0;
  }
}

Bytes::Writer::Writer(size_t capacity, base::File file)
    : Writer(file.IsValid() ? 0 : capacity) {
  file_ = std::move(file);
  if (file_.IsValid()) {
    stage_.reserve(kFileStageBytes);
  }
}

Bytes::Writer::~Writer() {
  if (pages_) {
    partition_alloc::FreePages(pages_, reserved_);
  }
}

bool Bytes::Writer::Append(base::span<const uint8_t> bytes) {
  if (file_.IsValid()) {
    while (!bytes.empty()) {
      const size_t take = std::min(kFileStageBytes - stage_.size(), bytes.size());
      const base::span<const uint8_t> part = bytes.first(take);
      stage_.insert(stage_.end(), part.begin(), part.end());
      bytes = bytes.subspan(take);
      size_ += take;
      if (stage_.size() == kFileStageBytes && !Flush()) {
        return false;
      }
    }
    return true;
  }
  if (pages_ && size_ + bytes.size() > reserved_ && !MoveToHeap()) {
    return false;
  }
  if (!pages_) {
    heap_.insert(heap_.end(), bytes.begin(), bytes.end());
    size_ = heap_.size();
    return true;
  }
  const size_t needed = size_ + bytes.size();
  if (needed > committed_) {
    const size_t page = partition_alloc::internal::SystemPageSize();
    const size_t commit_to =
        std::min(reserved_, RoundUp(std::max(needed, committed_ + kCommitStep),
                                    page));
    if (!partition_alloc::TryRecommitSystemPages(
            pages_ + committed_, commit_to - committed_,
            partition_alloc::PageAccessibilityConfiguration(
                partition_alloc::PageAccessibilityConfiguration::kReadWrite),
            partition_alloc::PageAccessibilityDisposition::kRequireUpdate)) {
      return false;
    }
    committed_ = commit_to;
  }
  PageSpan(pages_, size_, bytes.size()).copy_from(bytes);
  size_ = needed;
  return true;
}

bool Bytes::Writer::MoveToHeap() {
  // The one path that copies: the image outgrew what its encoder said it
  // could be. It keeps working; it merely stops being cheap.
  const base::span<const uint8_t> written = PageSpan(pages_, 0, size_);
  heap_.assign(written.begin(), written.end());
  partition_alloc::FreePages(pages_, reserved_);
  pages_ = 0;
  reserved_ = 0;
  committed_ = 0;
  return true;
}

bool Bytes::Writer::Flush() {
  // base::File::WriteAtCurrentPos may write less than asked, so loop; a
  // short write of zero is the disk saying no.
  base::span<const uint8_t> pending = stage_;
  while (!pending.empty()) {
    const std::optional<size_t> wrote = file_.WriteAtCurrentPos(pending);
    if (!wrote.has_value() || *wrote == 0) {
      return false;
    }
    pending = pending.subspan(*wrote);
  }
  stage_.clear();
  return true;
}

base::expected<Bytes, std::string> Bytes::Writer::Finish() {
  Bytes out;
  if (file_.IsValid()) {
    // The bytes are in the file; the caller reads size() for how many.
    const bool flushed = Flush();
    file_.Close();
    if (!flushed) {
      return base::unexpected("could not write the image to the file");
    }
    return out;
  }
  out.size_ = size_;
  if (!pages_) {
    out.heap_ = std::move(heap_);
    heap_.clear();
    size_ = 0;
    return out;
  }
  // Give back the committed tail past the last byte.
  const size_t page = partition_alloc::internal::SystemPageSize();
  const size_t keep = RoundUp(size_, page);
  if (committed_ > keep) {
    partition_alloc::DecommitSystemPages(
        pages_ + keep, committed_ - keep,
        partition_alloc::PageAccessibilityDisposition::kAllowKeepForPerf);
  }
  out.pages_ = std::exchange(pages_, 0);
  out.reserved_ = std::exchange(reserved_, 0);
  committed_ = 0;
  size_ = 0;
  return out;
}

}  // namespace shot
