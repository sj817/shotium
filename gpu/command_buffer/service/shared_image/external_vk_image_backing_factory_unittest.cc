// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image/external_vk_image_backing_factory.h"

#include <algorithm>
#include <memory>
#include <utility>

#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/functional/callback_helpers.h"
#include "build/build_config.h"
#include "cc/test/pixel_test_utils.h"
#include "components/viz/common/gpu/vulkan_in_process_context_provider.h"
#include "components/viz/common/resources/shared_image_format.h"
#include "gpu/command_buffer/service/service_utils.h"
#include "gpu/command_buffer/service/shared_context_state.h"
#include "gpu/command_buffer/service/shared_image/external_vk_image_skia_representation.h"
#include "gpu/command_buffer/service/shared_image/shared_image_factory.h"
#include "gpu/command_buffer/service/shared_image/shared_image_manager.h"
#include "gpu/command_buffer/service/shared_image/shared_image_representation.h"
#include "gpu/command_buffer/service/shared_image/shared_image_test_base.h"
#include "gpu/command_buffer/service/skia_utils.h"
#include "gpu/config/gpu_test_config.h"
#include "gpu/vulkan/vulkan_implementation.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/gpu/ganesh/GrBackendSemaphore.h"
#include "third_party/skia/include/gpu/ganesh/SkImageGanesh.h"
#include "third_party/skia/include/private/chromium/GrPromiseImageTexture.h"
#include "ui/gl/buildflags.h"


namespace gpu {
namespace {

class ExternalVkImageBackingFactoryTest : public SharedImageTestBase {
 protected:
  void SetUp() override {
    ASSERT_NO_FATAL_FAILURE(InitializeContext(GrContextType::kVulkan));
    constexpr bool kIsInterop = false;
    backing_factory_ = std::make_unique<ExternalVkImageBackingFactory>(
        context_state_, kIsInterop);
  }
};


class ExternalVkImageBackingFactoryWithFormatTest
    : public ExternalVkImageBackingFactoryTest,
      public testing::WithParamInterface<viz::SharedImageFormat> {
 public:
  viz::SharedImageFormat get_format() { return GetParam(); }
};

TEST_P(ExternalVkImageBackingFactoryWithFormatTest, Basic) {
  viz::SharedImageFormat format = get_format();
  auto mailbox = Mailbox::Generate();
  gfx::Size size(256, 256);
  auto color_space = gfx::ColorSpace::CreateSRGB();
  GrSurfaceOrigin surface_origin = kTopLeft_GrSurfaceOrigin;
  SkAlphaType alpha_type = kPremul_SkAlphaType;
  gpu::SharedImageUsageSet usage = SHARED_IMAGE_USAGE_DISPLAY_READ |
                                   SHARED_IMAGE_USAGE_GLES2_READ |
                                   SHARED_IMAGE_USAGE_GLES2_WRITE;

  bool supported = backing_factory_->CanCreateSharedImage(
      usage, format, size, /*thread_safe=*/false, gfx::EMPTY_BUFFER,
      GrContextType::kVulkan, {});
  ASSERT_TRUE(supported);

  // Verify backing can be created.
  auto backing = backing_factory_->CreateSharedImage(
      mailbox,
      {format, size, color_space, surface_origin, alpha_type, usage,
       "TestLabel"},
      gpu::kNullSurfaceHandle, /*is_thread_safe=*/false);
  ASSERT_TRUE(backing);

  std::unique_ptr<SharedImageRepresentationFactoryRef> shared_image =
      shared_image_manager_.Register(std::move(backing), &memory_type_tracker_);
  EXPECT_TRUE(shared_image);

  auto skia_representation = shared_image_representation_factory_.ProduceSkia(
      mailbox, context_state_.get());
  ASSERT_TRUE(skia_representation);

  {
    // Verify Skia write access works.
    std::vector<GrBackendSemaphore> begin_semaphores;
    std::vector<GrBackendSemaphore> end_semaphores;
    auto scoped_write_access = skia_representation->BeginScopedWriteAccess(
        &begin_semaphores, &end_semaphores,
        SharedImageRepresentation::AllowUnclearedAccess::kYes);
    ASSERT_TRUE(scoped_write_access);
    EXPECT_TRUE(begin_semaphores.empty());

    for (int plane = 0; plane < format.NumberOfPlanes(); ++plane) {
      auto* surface = scoped_write_access->surface(plane);
      ASSERT_TRUE(surface);

      auto plane_size = format.GetPlaneSize(plane, size);
      EXPECT_EQ(plane_size.width(), surface->width());
      EXPECT_EQ(plane_size.height(), surface->height());
    }

    scoped_write_access->ApplyBackendSurfaceEndState();
    // Handle end state and semaphores.
    if (!end_semaphores.empty()) {
      GrFlushInfo flush_info;
      if (!end_semaphores.empty()) {
        flush_info.fNumSemaphores = end_semaphores.size();
        flush_info.fSignalSemaphores = end_semaphores.data();
      }
      for (int plane = 0; plane < format.NumberOfPlanes(); ++plane) {
        gr_context()->flush(scoped_write_access->surface(plane), flush_info,
                            nullptr);
      }
      gr_context()->submit();
    }
  }

  // Must set cleared before read access.
  skia_representation->SetCleared();

  {
    // Verify Skia read access works.
    std::vector<GrBackendSemaphore> begin_semaphores;
    std::vector<GrBackendSemaphore> end_semaphores;
    auto scoped_read_access = skia_representation->BeginScopedReadAccess(
        &begin_semaphores, &end_semaphores);
    ASSERT_TRUE(scoped_read_access);

    for (int plane = 0; plane < format.NumberOfPlanes(); ++plane) {
      auto* promise_texture = scoped_read_access->promise_image_texture(plane);
      ASSERT_TRUE(promise_texture);
      GrBackendTexture backend_texture = promise_texture->backendTexture();
      EXPECT_TRUE(backend_texture.isValid());

      auto plane_size = format.GetPlaneSize(plane, size);
      EXPECT_EQ(plane_size.width(), backend_texture.width());
      EXPECT_EQ(plane_size.height(), backend_texture.height());
    }

    // Handle end state and semaphores.
    scoped_read_access->ApplyBackendSurfaceEndState();
    if (!end_semaphores.empty()) {
      GrFlushInfo flush_info = {
          .fNumSemaphores = end_semaphores.size(),
          .fSignalSemaphores = end_semaphores.data(),
      };
      gr_context()->flush(flush_info);
      gr_context()->submit();
    }
  }
  skia_representation.reset();

  // Verify GL access works.
  if (use_passthrough()) {
    auto gl_representation =
        shared_image_representation_factory_.ProduceGLTexturePassthrough(
            mailbox);
    ASSERT_TRUE(gl_representation);
    auto scoped_access = gl_representation->BeginScopedAccess(
        GL_SHARED_IMAGE_ACCESS_MODE_READ_CHROMIUM,
        SharedImageRepresentation::AllowUnclearedAccess::kNo);
    ASSERT_TRUE(scoped_access);

    for (int plane = 0; plane < format.NumberOfPlanes(); ++plane) {
      auto texture = gl_representation->GetTexturePassthrough(plane);
      ASSERT_TRUE(texture);
      EXPECT_NE(texture->service_id(), 0u);
    }
  } else {
    auto gl_representation =
        shared_image_representation_factory_.ProduceGLTexture(mailbox);
    ASSERT_TRUE(gl_representation);
    auto scoped_access = gl_representation->BeginScopedAccess(
        GL_SHARED_IMAGE_ACCESS_MODE_READ_CHROMIUM,
        SharedImageRepresentation::AllowUnclearedAccess::kNo);
    ASSERT_TRUE(scoped_access);

    for (int plane = 0; plane < format.NumberOfPlanes(); ++plane) {
      auto* texture = gl_representation->GetTexture(plane);
      ASSERT_TRUE(texture);
      EXPECT_NE(texture->service_id(), 0u);
    }
  }
}

// Verify that pixel upload works as expected.
TEST_P(ExternalVkImageBackingFactoryWithFormatTest, Upload) {
  viz::SharedImageFormat format = get_format();
  auto mailbox = Mailbox::Generate();
  gfx::Size size(30, 30);
  auto color_space = gfx::ColorSpace::CreateSRGB();
  GrSurfaceOrigin surface_origin = kTopLeft_GrSurfaceOrigin;
  SkAlphaType alpha_type = kPremul_SkAlphaType;
  gpu::SharedImageUsageSet usage =
      SHARED_IMAGE_USAGE_DISPLAY_READ | SHARED_IMAGE_USAGE_CPU_UPLOAD;

  // Verify backing can be created.
  auto backing = backing_factory_->CreateSharedImage(
      mailbox,
      {format, size, color_space, surface_origin, alpha_type, usage,
       "TestLabel"},
      gpu::kNullSurfaceHandle, /*is_thread_safe=*/false);
  ASSERT_TRUE(backing);

  std::vector<SkBitmap> bitmaps = AllocateRedBitmaps(format, size);

  // Upload pixels and set cleared.
  ASSERT_TRUE(backing->UploadFromMemory(GetSkPixmaps(bitmaps)));
  backing->SetCleared();

  std::unique_ptr<SharedImageRepresentationFactoryRef> shared_image_ref =
      shared_image_manager_.Register(std::move(backing), &memory_type_tracker_);
  ASSERT_TRUE(shared_image_ref);

  VerifyPixelsWithReadbackGanesh(mailbox, bitmaps);
}

TEST_P(ExternalVkImageBackingFactoryWithFormatTest, ReadbackToMemory) {
  viz::SharedImageFormat format = get_format();

  auto mailbox = Mailbox::Generate();
  gfx::Size size(9, 9);
  auto color_space = gfx::ColorSpace::CreateSRGB();
  GrSurfaceOrigin surface_origin = kTopLeft_GrSurfaceOrigin;
  SkAlphaType alpha_type = kPremul_SkAlphaType;
  gpu::SharedImageUsageSet usage =
      SHARED_IMAGE_USAGE_DISPLAY_READ | SHARED_IMAGE_USAGE_CPU_UPLOAD;
  gpu::SurfaceHandle surface_handle = gpu::kNullSurfaceHandle;

  bool supported = backing_factory_->CanCreateSharedImage(
      usage, format, size, /*thread_safe=*/false, gfx::EMPTY_BUFFER,
      GrContextType::kVulkan, {});
  ASSERT_TRUE(supported);

  auto backing = backing_factory_->CreateSharedImage(
      mailbox,
      {format, size, color_space, surface_origin, alpha_type, usage,
       "TestLabel"},
      surface_handle, /*is_thread_safe=*/false);
  ASSERT_TRUE(backing);

  std::vector<SkBitmap> src_bitmaps =
      AllocateRedBitmaps(format, size, /*added_stride=*/0);

  // Upload from bitmap with expected stride.
  ASSERT_TRUE(backing->UploadFromMemory(GetSkPixmaps(src_bitmaps)));

  const int num_planes = format.NumberOfPlanes();
  // Do readback into bitmap with same stride and validate pixels match what
  // was uploaded.
  std::vector<SkBitmap> readback_bitmaps(num_planes);
  for (int plane = 0; plane < num_planes; ++plane) {
    auto& info = src_bitmaps[plane].info();
    size_t stride = info.minRowBytes();
    readback_bitmaps[plane].allocPixels(info, stride);
  }

  std::vector<SkPixmap> pixmaps = GetSkPixmaps(readback_bitmaps);
  ASSERT_TRUE(backing->ReadbackToMemory(pixmaps));

  for (int plane = 0; plane < num_planes; ++plane) {
    EXPECT_TRUE(cc::MatchesBitmap(readback_bitmaps[plane], src_bitmaps[plane],
                                  cc::ExactPixelComparator()))
        << "plane_index=" << plane;
  }
}

std::string TestParamToString(
    const testing::TestParamInfo<viz::SharedImageFormat>& param_info) {
  return param_info.param.ToTestParamString();
}

const auto kSharedImageFormats =
    ::testing::Values(viz::SinglePlaneFormat::kRGBA_8888,
                      viz::SinglePlaneFormat::kBGRA_8888,
                      viz::SinglePlaneFormat::kR_8,
                      viz::SinglePlaneFormat::kRG_88,
                      viz::MultiPlaneFormat::kNV12,
                      viz::MultiPlaneFormat::kYV12,
                      viz::MultiPlaneFormat::kI420);

INSTANTIATE_TEST_SUITE_P(,
                         ExternalVkImageBackingFactoryWithFormatTest,
                         kSharedImageFormats,
                         TestParamToString);

}  // anonymous namespace
}  // namespace gpu
