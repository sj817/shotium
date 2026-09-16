// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_FRAME_CLIENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_FRAME_CLIENT_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"

namespace blink {

enum class FrameDetachType;

class CORE_EXPORT FrameClient : public GarbageCollected<FrameClient> {
 public:
  virtual bool InShadowTree() const { return false; }

  virtual void Detached(FrameDetachType) {}

  virtual unsigned BackForwardLength() { return 0; }

  virtual ~FrameClient() = default;

  virtual void Trace(Visitor* visitor) const {}
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_FRAME_CLIENT_H_
