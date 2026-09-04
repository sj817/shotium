// Copyright 2026 The Shot Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_BYTES_H_
#define SHOT_SHOT_BYTES_H_

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/types/expected.h"

namespace shot {

// Bytes the engine produced and owns -- an encoded image, most of the time.
// Move-only, and read as a span.
//
// An image is written through a Writer, which reserves address space for the
// largest the image could be and commits it a step at a time as the encoder
// fills it. The alternative, a vector that doubles, would have held a 67 MB
// PNG three times over at its last growth step; this holds it once, at its
// final size, from the first byte.
class Bytes {
 public:
  class Writer;

  Bytes();
  Bytes(Bytes&& other) noexcept;
  Bytes& operator=(Bytes&& other) noexcept;
  Bytes(const Bytes&) = delete;
  Bytes& operator=(const Bytes&) = delete;
  ~Bytes();

  // A copy on the heap, for the small things: messages, tests.
  static Bytes Copy(base::span<const uint8_t> bytes);
  // Takes a vector as it is.
  static Bytes FromVector(std::vector<uint8_t> bytes);

  base::span<const uint8_t> span() const;
  const uint8_t* data() const { return span().data(); }
  size_t size() const { return size_; }
  bool empty() const { return size_ == 0; }
  // So that anything taking a span -- base::WriteFile, a frame writer --
  // takes these.
  operator base::span<const uint8_t>() const { return span(); }  // NOLINT

 private:
  void Release();

  // One of the two is in use: the vector, or the reserved pages.
  std::vector<uint8_t> heap_;
  uintptr_t pages_ = 0;
  size_t reserved_ = 0;
  size_t size_ = 0;
};

// Builds a Bytes of unknown final length without holding it twice.
class Bytes::Writer {
 public:
  // `capacity` is the most the result can be; more than that and the writer
  // falls back to a vector, which still works and merely costs what a vector
  // costs. If the reservation itself fails the writer starts on the vector.
  explicit Writer(size_t capacity);
  // The same, but when `file` is open the bytes go straight into it and
  // nothing is held: Finish() returns an empty Bytes and size() says how much
  // was written. An image that is going to a file need never exist in memory
  // as a whole. With an invalid `file` this is Writer(capacity).
  Writer(size_t capacity, base::File file);
  Writer(const Writer&) = delete;
  Writer& operator=(const Writer&) = delete;
  ~Writer();

  bool Append(base::span<const uint8_t> bytes);
  size_t size() const { return size_; }
  bool writes_to_file() const { return file_.IsValid(); }

  // The bytes written so far, with the memory past them given back. In file
  // mode it is the last of the bytes reaching the disk, which is where a full
  // disk finally says so.
  base::expected<Bytes, std::string> Finish();

 private:
  bool MoveToHeap();
  // File mode: writes what is staged.
  bool Flush();

  base::File file_;
  // File mode: the bytes not yet written. Encoders hand over a few kilobytes
  // at a time -- libpng writes each IDAT chunk as a header, the data and a
  // CRC -- and a system call for each of those cost a 67 MB PNG four hundred
  // milliseconds; so they are collected here and written in runs.
  std::vector<uint8_t> stage_;
  std::vector<uint8_t> heap_;
  uintptr_t pages_ = 0;
  size_t reserved_ = 0;
  size_t committed_ = 0;
  size_t size_ = 0;
};

}  // namespace shot

#endif  // SHOT_SHOT_BYTES_H_
