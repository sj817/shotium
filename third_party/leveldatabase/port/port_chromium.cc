// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "port/port_chromium.h"

#include <cstddef>
#include <cstdint>
#include <string>

#include "third_party/snappy/src/snappy.h"

namespace leveldb {
namespace port {

bool Snappy_Compress(const char* input,
                     size_t input_length,
                     std::string* output) {
  output->resize(snappy::MaxCompressedLength(input_length));
  size_t outlen;
  snappy::RawCompress(input, input_length, &(*output)[0], &outlen);
  output->resize(outlen);
  return true;
}

bool Snappy_GetUncompressedLength(const char* input_data,
                                  size_t input_length,
                                  size_t* result) {
  return snappy::GetUncompressedLength(input_data, input_length, result);
}

bool Snappy_Uncompress(const char* input_data,
                       size_t input_length,
                       char* output) {
  return snappy::RawUncompress(input_data, input_length, output);
}

uint32_t AcceleratedCRC32C(uint32_t crc, const char* buf, size_t size) {
  // This used to forward to //third_party/crc32c, which picks an SSE4.2 or
  // ARMv8 CRC32 implementation at runtime. That library is a gclient-managed
  // submodule and is not in this checkout.
  //
  // Returning zero is not a stub: it is leveldb's own signal for "this CPU
  // cannot accelerate CRC32C". CanAccelerateCRC32C() in src/util/crc32c.cc
  // calls this with a known buffer and compares against the expected checksum,
  // so a zero here makes Extend() use leveldb's portable table implementation
  // in that same file. The checksums are identical; only the throughput of
  // computing them differs.
  return 0;
}

}  // namespace port
}  // namespace leveldb
