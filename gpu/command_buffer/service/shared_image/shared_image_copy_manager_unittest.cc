// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image/shared_image_copy_manager.h"

#include "build/build_config.h"
#include "gpu/command_buffer/service/shared_image/cpu_readback_upload_copy_strategy.h"
#include "gpu/command_buffer/service/shared_image/shared_image_test_base.h"
#include "gpu/command_buffer/service/shared_image/shared_memory_copy_strategy.h"
#include "gpu/command_buffer/service/shared_image/shared_memory_image_backing.h"
#include "gpu/command_buffer/service/shared_image/shared_memory_image_backing_factory.h"
#include "gpu/command_buffer/service/shared_image/test_image_backing.h"
#include "gpu/command_buffer/service/shared_image/wrapped_sk_image_backing_factory.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gl/test/gl_surface_test_support.h"

namespace gpu {

class SharedImageCopyManagerTest : public SharedImageTestBase {
 public:
  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(InitializeContext(GrContextType::kGL));
    copy_manager_ = base::MakeRefCounted<SharedImageCopyManager>();
  }

 protected:
  scoped_refptr<SharedImageCopyManager> copy_manager_;
};

TEST_F(SharedImageCopyManagerTest, CopyUsingCpuFallbackStrategy) {
  copy_manager_->AddStrategy(std::make_unique<CPUReadbackUploadCopyStrategy>());

  auto src_backing = std::make_unique<TestImageBacking>(
      Mailbox::Generate(),
      SharedImageInfo(
          viz::SinglePlaneFormat::kRGBA_8888, gfx::Size(100, 100),
          gfx::ColorSpace::CreateSRGB(), kTopLeft_GrSurfaceOrigin,
          kPremul_SkAlphaType,
          SHARED_IMAGE_USAGE_CPU_READ | SHARED_IMAGE_USAGE_CPU_WRITE_ONLY,
          "TestLabel"),
      1024);
  auto dst_backing = std::make_unique<TestImageBacking>(
      Mailbox::Generate(),
      SharedImageInfo(
          viz::SinglePlaneFormat::kRGBA_8888, gfx::Size(100, 100),
          gfx::ColorSpace::CreateSRGB(), kTopLeft_GrSurfaceOrigin,
          kPremul_SkAlphaType,
          SHARED_IMAGE_USAGE_CPU_READ | SHARED_IMAGE_USAGE_CPU_WRITE_ONLY,
          "TestLabel"),
      1024);

  EXPECT_TRUE(copy_manager_->CopyImage(src_backing.get(), dst_backing.get()));

  EXPECT_TRUE(src_backing->GetReadbackToMemoryCalledAndReset());
  EXPECT_TRUE(dst_backing->GetUploadFromMemoryCalledAndReset());
}

TEST_F(SharedImageCopyManagerTest, CopyUsingSharedMemoryStrategy) {
  copy_manager_->AddStrategy(std::make_unique<SharedMemoryCopyStrategy>());

  constexpr viz::SharedImageFormat format = viz::SinglePlaneFormat::kRGBA_8888;
  constexpr gfx::Size size(100, 100);
  constexpr auto usage =
      SHARED_IMAGE_USAGE_CPU_READ | SHARED_IMAGE_USAGE_CPU_WRITE_ONLY;

  // Create a shared memory backing.
  auto shm_backing = SharedMemoryImageBackingFactory().CreateSharedImage(
      Mailbox::Generate(),
      {format, size, gfx::ColorSpace::CreateSRGB(), kTopLeft_GrSurfaceOrigin,
       kPremul_SkAlphaType, usage, "TestLabel"},
      kNullSurfaceHandle,
      /*is_thread_safe=*/false, gfx::BufferUsage::GPU_READ_CPU_READ_WRITE);

  // Create a generic test backing.
  auto test_backing = std::make_unique<TestImageBacking>(
      Mailbox::Generate(),
      SharedImageInfo(format, size, gfx::ColorSpace::CreateSRGB(),
                      kTopLeft_GrSurfaceOrigin, kPremul_SkAlphaType, usage,
                      "TestLabel"),
      1024);

  // Test copy from Shared Memory to Test backing.
  EXPECT_TRUE(copy_manager_->CopyImage(shm_backing.get(), test_backing.get()));
  EXPECT_TRUE(test_backing->GetUploadFromMemoryCalledAndReset());
  EXPECT_FALSE(test_backing->GetReadbackToMemoryCalledAndReset());

  // Test copy from Test to Shared Memory backing.
  EXPECT_TRUE(copy_manager_->CopyImage(test_backing.get(), shm_backing.get()));
  EXPECT_TRUE(test_backing->GetReadbackToMemoryCalledAndReset());
  EXPECT_FALSE(test_backing->GetUploadFromMemoryCalledAndReset());
}

#if 0
class SharedImageCopyManagerTestWithGraphiteDawn : public SharedImageTestBase {
 public:
  void SetUp() override {
    // This will set up Dawn and Graphite.
    ASSERT_NO_FATAL_FAILURE(InitializeContext(GrContextType::kGraphiteDawn));
    copy_manager_ = base::MakeRefCounted<SharedImageCopyManager>();
  }

 protected:
  scoped_refptr<SharedImageCopyManager> copy_manager_;
};

TEST_F(SharedImageCopyManagerTestWithGraphiteDawn, CopyUsingDawnStrategy) {
  copy_manager_->AddStrategy(std::make_unique<DawnCopyStrategy>());

  constexpr viz::SharedImageFormat format = viz::SinglePlaneFormat::kRGBA_8888;
  constexpr gfx::Size size(100, 100);
  constexpr auto dawn_backing_usage = SharedImageUsageSet(
      SHARED_IMAGE_USAGE_WEBGPU_READ | SHARED_IMAGE_USAGE_WEBGPU_WRITE);
  constexpr auto graphite_backing_usage = SharedImageUsageSet(
      SHARED_IMAGE_USAGE_DISPLAY_READ | SHARED_IMAGE_USAGE_DISPLAY_WRITE |
      SHARED_IMAGE_USAGE_RASTER_READ | SHARED_IMAGE_USAGE_RASTER_WRITE);

  // Create a Dawn backing.
  DawnImageBackingFactory dawn_factory;
  auto dawn_backing = dawn_factory.CreateSharedImage(
      Mailbox::Generate(),
      {format, size, gfx::ColorSpace::CreateSRGB(), kTopLeft_GrSurfaceOrigin,
       kPremul_SkAlphaType, dawn_backing_usage, "dawn_backing"},
      kNullSurfaceHandle, /*is_thread_safe=*/false);
  ASSERT_TRUE(dawn_backing);

  static_cast<DawnImageBacking*>(dawn_backing.get())
      ->InitializeForTesting(dawn_context_provider_->GetDevice());

  // Create a Graphite backing.
  WrappedSkImageBackingFactory sk_image_factory(context_state_);
  auto graphite_backing = sk_image_factory.CreateSharedImage(
      Mailbox::Generate(),
      {format, size, gfx::ColorSpace::CreateSRGB(), kTopLeft_GrSurfaceOrigin,
       kPremul_SkAlphaType, graphite_backing_usage, "graphite_backing"},
      kNullSurfaceHandle, /*is_thread_safe=*/false);
  ASSERT_TRUE(graphite_backing);

  // Test copy from Graphite to Dawn.
  EXPECT_TRUE(
      copy_manager_->CopyImage(graphite_backing.get(), dawn_backing.get()));

  // Test copy from Dawn to Graphite.
  EXPECT_TRUE(
      copy_manager_->CopyImage(dawn_backing.get(), graphite_backing.get()));
}

class DawnCopyStrategyStackUARTest : public testing::Test {
 public:
  void SetUp() override {


    ASSERT_GT(adapters.size(), 0u);

    device_descriptor.requiredFeatureCount = 1;
    device_descriptor.requiredFeatures = &dawn_internal_usage;

    ASSERT_TRUE(dawn_device_) << "Failed to create Dawn device";
  }

  void TearDown() override {
    dawn_instance_.reset();
  }

 protected:
};

TEST_F(DawnCopyStrategyStackUARTest,
       MapAsyncCallbackOutlivesStackOnWaitAnyFailure) {
  texture_desc.size = {4, 4, 1};
  ASSERT_TRUE(src_texture);

  auto dst_backing = std::make_unique<TestImageBacking>(
      Mailbox::Generate(),
      SharedImageInfo(
          viz::SinglePlaneFormat::kRGBA_8888, gfx::Size(4, 4),
          gfx::ColorSpace::CreateSRGB(), kTopLeft_GrSurfaceOrigin,
          kPremul_SkAlphaType,
          SHARED_IMAGE_USAGE_CPU_READ | SHARED_IMAGE_USAGE_CPU_WRITE_ONLY,
          "TestLabel"),
      1024);

  EXPECT_FALSE(DawnCopyStrategy::CopyFromTextureToBacking(
      src_texture, dst_backing.get(), dawn_device_));
}

#endif  // 0

}  // namespace gpu
