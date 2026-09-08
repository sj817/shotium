// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MODULES_INITIALIZER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MODULES_INITIALIZER_H_

#include "third_party/blink/renderer/core/core_initializer.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

// Supplies the remaining optional modules hooks to BlinkInitializer.
class MODULES_EXPORT ModulesInitializer : public CoreInitializer {
 public:
  void Initialize() override;

 private:
  void InstallSupplements(LocalFrame&) const override;
  PictureInPictureController* CreatePictureInPictureController(
      Document&) const override;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MODULES_INITIALIZER_H_
