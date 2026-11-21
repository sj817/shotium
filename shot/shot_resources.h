// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_RESOURCES_H_
#define SHOT_SHOT_RESOURCES_H_

#include <stdint.h>

#include "base/containers/span.h"

namespace shot {

// The resource packs, compiled into the binary by //shot:shot_embedded_paks.
//
// They used to ship as two files beside the binary and were opened with
// base::MemoryMappedFile, which maps MAP_SHARED even read-only. FUSE in
// direct-io mode -- virtiofs with cache=none, the usual way a VM shares a host
// folder -- refuses shared mappings, so an engine installed there failed a
// CHECK inside ResourceBundle before it could report anything. Bytes in
// .rodata cannot fail to load, and the binary cannot meet packs from another
// version. The two packs are under 40 KB together.
base::span<const uint8_t> ShotDataPak();
base::span<const uint8_t> ShotStringsPak();

}  // namespace shot

#endif  // SHOT_SHOT_RESOURCES_H_
