// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <vector>

#include "gpu/config/gpu_info_collector.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/init/gl_factory.h"

namespace gpu {

#if 0
TEST(GPUConfig, CollectGraphicsInfoDawnGLES) {
  EGLContext contextBefore = eglGetCurrentContext();

  GpuPreferences gpu_preferences;
  std::vector<std::string> dawn_info_list;
  CollectDawnInfo(gpu_preferences, true, &dawn_info_list);

  CollectDawnInfo(gpu_preferences, false, &dawn_info_list);
  EGLContext contextAfter = eglGetCurrentContext();
  EXPECT_TRUE(contextBefore == contextAfter);
}
#endif  // 0

}  // namespace gpu
