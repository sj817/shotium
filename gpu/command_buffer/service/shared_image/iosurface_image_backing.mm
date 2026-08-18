// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image/iosurface_image_backing.h"

#include <EGL/egl.h>
#include <EGL/eglext.h>
#import <Metal/Metal.h>

#include "base/apple/scoped_cftyperef.h"
#include "base/apple/scoped_nsobject.h"
#include "base/bits.h"
#include "base/compiler_specific.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/scoped_policy.h"
#include "base/notimplemented.h"
#include "base/trace_event/memory_dump_manager.h"
#include "components/viz/common/resources/shared_image_format_utils.h"
#include "gpu/command_buffer/common/shared_image_info.h"
#include "gpu/command_buffer/common/shared_image_trace_utils.h"
#include "gpu/command_buffer/common/shared_image_usage.h"
#include "gpu/command_buffer/service/shared_context_state.h"
#include "gpu/command_buffer/service/shared_image/copy_image_plane.h"
#include "gpu/command_buffer/service/shared_image/iosurface_image_backing_factory.h"
#include "gpu/command_buffer/service/shared_image/shared_image_format_service_utils.h"
#include "gpu/command_buffer/service/shared_image/shared_image_gl_utils.h"
#include "gpu/command_buffer/service/skia_utils.h"
#include "third_party/angle/include/EGL/eglext_angle.h"
#include "third_party/skia/include/core/SkColorSpace.h"
#include "third_party/skia/include/gpu/ganesh/GrContextThreadSafeProxy.h"
#include "third_party/skia/include/gpu/ganesh/SkSurfaceGanesh.h"
#include "third_party/skia/include/gpu/graphite/Recorder.h"
#include "third_party/skia/include/gpu/graphite/Surface.h"
#include "third_party/skia/include/private/chromium/GrPromiseImageTexture.h"
#include "ui/gfx/mac/mtl_shared_event_fence.h"
#include "ui/gl/egl_surface_io_surface.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_display.h"
#include "ui/gl/gl_display_manager.h"
#include "ui/gl/gl_fence_egl.h"
#include "ui/gl/gl_gl_api_implementation.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/scoped_binders.h"
#include "ui/gl/scoped_make_current.h"
#include "ui/gl/scoped_restore_texture.h"

namespace gpu {

namespace {
using GraphiteTextureHolder = SkiaImageRepresentation::GraphiteTextureHolder;

// Alignment required by `CopyBufferToTexture` and `CopyTextureToBuffer`.
constexpr uint32_t kTextureBytesPerRowAlignment = 256u;

struct ScopedIOSurfaceLock {
  ScopedIOSurfaceLock(IOSurfaceRef iosurface, IOSurfaceLockOptions options)
      : io_surface_(iosurface), options_(options) {
    kern_return_t r = IOSurfaceLock(io_surface_, options_, /*seed=*/nullptr);
    CHECK_EQ(KERN_SUCCESS, r);
  }
  ~ScopedIOSurfaceLock() {
    kern_return_t r = IOSurfaceUnlock(io_surface_, options_, /*seed=*/nullptr);
    CHECK_EQ(KERN_SUCCESS, r);
  }

  ScopedIOSurfaceLock(const ScopedIOSurfaceLock&) = delete;
  ScopedIOSurfaceLock& operator=(const ScopedIOSurfaceLock&) = delete;

 private:
  const IOSurfaceRef io_surface_;
  const IOSurfaceLockOptions options_;
};

// Returns planar format for given multiplanar `format`.
viz::SharedImageFormat GetFormatForPlane(viz::SharedImageFormat format,
                                         int plane) {
  DCHECK(format.is_multi_plane());
  DCHECK(format.IsValidPlaneIndex(plane));

  // IOSurfaceBacking does not support external sampler use cases.
  int num_channels = format.NumChannelsInPlane(plane);
  DCHECK_LE(num_channels, 2);
  switch (format.channel_format()) {
    case viz::SharedImageFormat::ChannelFormat::k8:
      return num_channels == 2 ? viz::SinglePlaneFormat::kRG_88
                               : viz::SinglePlaneFormat::kR_8;
    case viz::SharedImageFormat::ChannelFormat::k10:
    case viz::SharedImageFormat::ChannelFormat::k16:
    case viz::SharedImageFormat::ChannelFormat::k16F:
      return num_channels == 2 ? viz::SinglePlaneFormat::kRG_1616
                               : viz::SinglePlaneFormat::kR_16;
  }
  NOTREACHED();
}


id<MTLDevice> QueryMetalDeviceFromANGLE(EGLDisplay display) {
  auto* display_egl =
      gl::GLDisplayManagerEGL::GetInstance()->GetDisplay(display);
  return display_egl ? display_egl->GetMetalDevice() : nil;
}

}  // namespace

// DawnBufferCopyRepresentation and WebNNIOSurfaceTensorRepresentation are
// gone: Dawn was removed with WebGPU, and //services/webnn with the ML
// stack.

///////////////////////////////////////////////////////////////////////////////
// IOSurfaceBackingEGLState

IOSurfaceBackingEGLState::IOSurfaceBackingEGLState(
    Client* client,
    EGLDisplay egl_display,
    gl::GLContext* gl_context,
    gl::GLSurface* gl_surface,
    GLuint gl_target,
    std::vector<scoped_refptr<gles2::TexturePassthrough>> gl_textures)
    : client_(client),
      egl_display_(egl_display),
      context_(gl_context),
      surface_(gl_surface),
      gl_target_(gl_target),
      gl_textures_(std::move(gl_textures)),
      created_task_runner_(base::SingleThreadTaskRunner::GetCurrentDefault()) {
  client_->IOSurfaceBackingEGLStateBeingCreated(this);
}

IOSurfaceBackingEGLState::~IOSurfaceBackingEGLState() {
  // To use ui::ScopedMakeCurrent, this funciton must be called on the same
  // thread where it got its current context context during creation.
  ui::ScopedMakeCurrent smc(context_.get(), surface_.get());
  if (client_) {
    // IOSurfaceBackingEGLState is destroyed directly from the same thread.
    client_->IOSurfaceBackingEGLStateBeingDestroyed(this, !context_lost_);
    DCHECK(gl_textures_.empty());
  } else {
    // ~IOSurfaceBackingEGLState is posted from the other thread when the
    // client_ (IOSurfaceImageBacking) was destroyed.
    if (context_lost_) {
      for (const auto& texture : gl_textures_) {
        texture->MarkContextLost();
      }
    }
    gl_textures_.clear();
    egl_surfaces_.clear();
  }
}

GLuint IOSurfaceBackingEGLState::GetGLServiceId(size_t plane_index) const {
  return GetGLTexture(plane_index)->service_id();
}

bool IOSurfaceBackingEGLState::BeginAccess(bool readonly) {
  gl::GLDisplayEGL* display = gl::GLDisplayEGL::GetDisplayForCurrentContext();
  CHECK(display);
  CHECK(display->GetDisplay() == egl_display_);
  return client_->IOSurfaceBackingEGLStateBeginAccess(this, readonly);
}

void IOSurfaceBackingEGLState::EndAccess(bool readonly) {
  client_->IOSurfaceBackingEGLStateEndAccess(this, readonly);
}

void IOSurfaceBackingEGLState::WillRelease(bool have_context) {
  if (!have_context) {
    context_lost_ = true;
  }
}

void IOSurfaceBackingEGLState::RemoveClient() {
  client_ = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// GLTextureIRepresentation
class IOSurfaceImageBacking::GLTextureIRepresentation final
    : public GLTexturePassthroughImageRepresentation {
 public:
  GLTextureIRepresentation(SharedImageManager* manager,
                           SharedImageBacking* backing,
                           scoped_refptr<IOSurfaceBackingEGLState> egl_state,
                           MemoryTypeTracker* tracker)
      : GLTexturePassthroughImageRepresentation(manager, backing, tracker),
        egl_state_(egl_state) {}
  ~GLTextureIRepresentation() override {
    egl_state_->WillRelease(has_context());
    AutoLock auto_lock(backing());
    egl_state_.reset();
  }

 private:
  // GLTexturePassthroughImageRepresentation:
  const scoped_refptr<gles2::TexturePassthrough>& GetTexturePassthrough(
      size_t plane_index) override {
    return egl_state_->GetGLTexture(plane_index);
  }

  bool BeginAccess(GLenum mode) override {
    DCHECK(mode_ == 0);
    AutoLock auto_lock(backing());
    mode_ = mode;
    bool readonly = mode_ != GL_SHARED_IMAGE_ACCESS_MODE_READWRITE_CHROMIUM;
    return egl_state_->BeginAccess(readonly);
  }

  void EndAccess() override {
    DCHECK(mode_ != 0);
    AutoLock auto_lock(backing());
    GLenum current_mode = mode_;
    mode_ = 0;
    egl_state_->EndAccess(current_mode !=
                          GL_SHARED_IMAGE_ACCESS_MODE_READWRITE_CHROMIUM);
  }

  scoped_refptr<IOSurfaceBackingEGLState> egl_state_;
  GLenum mode_ = 0;
};

///////////////////////////////////////////////////////////////////////////////
// SkiaGaneshRepresentation

class IOSurfaceImageBacking::SkiaGaneshRepresentation final
    : public SkiaGaneshImageRepresentation {
 public:
  SkiaGaneshRepresentation(
      SharedImageManager* manager,
      SharedImageBacking* backing,
      scoped_refptr<IOSurfaceBackingEGLState> egl_state,
      scoped_refptr<SharedContextState> context_state,
      std::vector<sk_sp<GrPromiseImageTexture>> promise_textures,
      MemoryTypeTracker* tracker);
  ~SkiaGaneshRepresentation() override;

 private:
  // SkiaGaneshImageRepresentation:
  std::vector<sk_sp<SkSurface>> BeginWriteAccess(
      int final_msaa_count,
      const SkSurfaceProps& surface_props,
      const gfx::Rect& update_rect,
      std::vector<GrBackendSemaphore>* begin_semaphores,
      std::vector<GrBackendSemaphore>* end_semaphores,
      std::unique_ptr<skgpu::MutableTextureState>* end_state) override;
  std::vector<sk_sp<GrPromiseImageTexture>> BeginWriteAccess(
      std::vector<GrBackendSemaphore>* begin_semaphores,
      std::vector<GrBackendSemaphore>* end_semaphore,
      std::unique_ptr<skgpu::MutableTextureState>* end_state) override;
  void EndWriteAccess() override;
  std::vector<sk_sp<GrPromiseImageTexture>> BeginReadAccess(
      std::vector<GrBackendSemaphore>* begin_semaphores,
      std::vector<GrBackendSemaphore>* end_semaphores,
      std::unique_ptr<skgpu::MutableTextureState>* end_state) override;
  void EndReadAccess() override;
  bool SupportsMultipleConcurrentReadAccess() override;

  void CheckContext();

  scoped_refptr<IOSurfaceBackingEGLState> egl_state_;
  const scoped_refptr<SharedContextState> context_state_;
  std::vector<sk_sp<GrPromiseImageTexture>> promise_textures_;
  std::vector<sk_sp<SkSurface>> write_surfaces_;
#if DCHECK_IS_ON()
  raw_ptr<gl::GLContext> context_ = nullptr;
#endif
};

IOSurfaceImageBacking::SkiaGaneshRepresentation::SkiaGaneshRepresentation(
    SharedImageManager* manager,
    SharedImageBacking* backing,
    scoped_refptr<IOSurfaceBackingEGLState> egl_state,
    scoped_refptr<SharedContextState> context_state,
    std::vector<sk_sp<GrPromiseImageTexture>> promise_textures,
    MemoryTypeTracker* tracker)
    : SkiaGaneshImageRepresentation(context_state->gr_context(),
                                    manager,
                                    backing,
                                    tracker),
      egl_state_(egl_state),
      context_state_(std::move(context_state)),
      promise_textures_(promise_textures) {
  DCHECK(!promise_textures_.empty());
#if DCHECK_IS_ON()
  if (context_state_->GrContextIsGL())
    context_ = gl::GLContext::GetCurrent();
#endif
}

IOSurfaceImageBacking::SkiaGaneshRepresentation::~SkiaGaneshRepresentation() {
  if (!write_surfaces_.empty()) {
    DLOG(ERROR) << "SkiaImageRepresentation was destroyed while still "
                << "open for write access.";
  }

  promise_textures_.clear();
  if (egl_state_) {
    DCHECK(context_state_->GrContextIsGL());
    egl_state_->WillRelease(has_context());

    AutoLock auto_lock(backing());
    egl_state_.reset();
  }
}

std::vector<sk_sp<SkSurface>>
IOSurfaceImageBacking::SkiaGaneshRepresentation::BeginWriteAccess(
    int final_msaa_count,
    const SkSurfaceProps& surface_props,
    const gfx::Rect& update_rect,
    std::vector<GrBackendSemaphore>* begin_semaphores,
    std::vector<GrBackendSemaphore>* end_semaphores,
    std::unique_ptr<skgpu::MutableTextureState>* end_state) {
  AutoLock auto_lock(backing());
  CheckContext();

  if (egl_state_) {
    DCHECK(context_state_->GrContextIsGL());
    if (!egl_state_->BeginAccess(/*readonly=*/false)) {
      return {};
    }
  }

  if (!write_surfaces_.empty()) {
    return {};
  }

  if (promise_textures_.empty()) {
    return {};
  }

  DCHECK_EQ(static_cast<int>(promise_textures_.size()),
            format().NumberOfPlanes());
  std::vector<sk_sp<SkSurface>> surfaces;
  for (int plane_index = 0; plane_index < format().NumberOfPlanes();
       plane_index++) {
    // Use the color type per plane for multiplanar formats.
    SkColorType sk_color_type =
        viz::ToClosestSkColorType(format(), plane_index);
    // Gray is not a renderable single channel format, but alpha is.
    if (sk_color_type == kGray_8_SkColorType) {
      sk_color_type = kAlpha_8_SkColorType;
    }
    auto surface = SkSurfaces::WrapBackendTexture(
        context_state_->gr_context(),
        promise_textures_[plane_index]->backendTexture(), surface_origin(),
        final_msaa_count, sk_color_type,
        backing()->color_space().GetAsFullRangeRGB().ToSkColorSpace(),
        &surface_props);
    if (!surface) {
      return {};
    }
    surfaces.push_back(surface);
  }

  write_surfaces_ = surfaces;
  return surfaces;
}

std::vector<sk_sp<GrPromiseImageTexture>>
IOSurfaceImageBacking::SkiaGaneshRepresentation::BeginWriteAccess(
    std::vector<GrBackendSemaphore>* begin_semaphores,
    std::vector<GrBackendSemaphore>* end_semaphores,
    std::unique_ptr<skgpu::MutableTextureState>* end_state) {
  AutoLock auto_lock(backing());
  CheckContext();

  if (egl_state_) {
    DCHECK(context_state_->GrContextIsGL());
    if (!egl_state_->BeginAccess(/*readonly=*/false)) {
      return {};
    }
  }
  if (promise_textures_.empty()) {
    return {};
  }
  return promise_textures_;
}

void IOSurfaceImageBacking::SkiaGaneshRepresentation::EndWriteAccess() {
  AutoLock auto_lock(backing());
#if DCHECK_IS_ON()
  for (auto& surface : write_surfaces_) {
    DCHECK(surface->unique());
  }
#endif

  CheckContext();
  write_surfaces_.clear();

  if (egl_state_)
    egl_state_->EndAccess(/*readonly=*/false);
}

std::vector<sk_sp<GrPromiseImageTexture>>
IOSurfaceImageBacking::SkiaGaneshRepresentation::BeginReadAccess(
    std::vector<GrBackendSemaphore>* begin_semaphores,
    std::vector<GrBackendSemaphore>* end_semaphores,
    std::unique_ptr<skgpu::MutableTextureState>* end_state) {
  AutoLock auto_lock(backing());
  CheckContext();

  if (egl_state_) {
    DCHECK(context_state_->GrContextIsGL());
    if (!egl_state_->BeginAccess(/*readonly=*/true)) {
      return {};
    }
  }
  if (promise_textures_.empty()) {
    return {};
  }
  return promise_textures_;
}

void IOSurfaceImageBacking::SkiaGaneshRepresentation::EndReadAccess() {
  AutoLock auto_lock(backing());

  if (egl_state_) {
    egl_state_->EndAccess(/*readonly=*/true);
  }
}

bool IOSurfaceImageBacking::SkiaGaneshRepresentation::
    SupportsMultipleConcurrentReadAccess() {
  return true;
}

void IOSurfaceImageBacking::SkiaGaneshRepresentation::CheckContext() {
#if DCHECK_IS_ON()
  if (!context_state_->context_lost() && context_)
    DCHECK(gl::GLContext::GetCurrent() == context_);
#endif
}

///////////////////////////////////////////////////////////////////////////////
// OverlayRepresentation

class IOSurfaceImageBacking::OverlayRepresentation final
    : public OverlayImageRepresentation {
 public:
  OverlayRepresentation(SharedImageManager* manager,
                        SharedImageBacking* backing,
                        MemoryTypeTracker* tracker,
                        gfx::ScopedIOSurface io_surface)
      : OverlayImageRepresentation(manager, backing, tracker),
        io_surface_(std::move(io_surface)) {}
  ~OverlayRepresentation() override = default;

 private:
  bool BeginReadAccess(gfx::GpuFenceHandle& acquire_fence) override;
  void EndReadAccess(gfx::GpuFenceHandle release_fence) override;
  gfx::ScopedIOSurface GetIOSurface() const override;
  std::vector<gfx::MTLSharedEventFence> GetBackpressureFences()
      const override;
  bool IsInUseByWindowServer() const override;

  gfx::ScopedIOSurface io_surface_;
};

bool IOSurfaceImageBacking::OverlayRepresentation::BeginReadAccess(
    gfx::GpuFenceHandle& acquire_fence) {
  auto* iosurface_backing = static_cast<IOSurfaceImageBacking*>(backing());
  AutoLock auto_lock(iosurface_backing);

  if (!iosurface_backing->BeginAccess(/*readonly=*/true)) {
    return false;
  }

  // This will transition the image to be accessed by CoreAnimation.
  iosurface_backing->WaitForCommandsToBeScheduled();

  return true;
}

void IOSurfaceImageBacking::OverlayRepresentation::EndReadAccess(
    gfx::GpuFenceHandle release_fence) {
  auto* iosurface_backing = static_cast<IOSurfaceImageBacking*>(backing());
  AutoLock auto_lock(iosurface_backing);
  DCHECK(release_fence.is_null());

  iosurface_backing->EndAccess(/*readonly=*/true);
}

gfx::ScopedIOSurface
IOSurfaceImageBacking::OverlayRepresentation::GetIOSurface() const {
  return io_surface_;
}

std::vector<gfx::MTLSharedEventFence>
IOSurfaceImageBacking::OverlayRepresentation::GetBackpressureFences() const {
  auto* iosurface_backing = static_cast<IOSurfaceImageBacking*>(backing());
  AutoLock auto_lock(iosurface_backing);

  std::vector<gfx::MTLSharedEventFence> backpressure_fences;
  for (const auto& [shared_event, signaled_value] :
       iosurface_backing->exclusive_shared_events_) {
    backpressure_fences.emplace_back(shared_event.get(), signaled_value);
  }
  return backpressure_fences;
}

bool IOSurfaceImageBacking::OverlayRepresentation::IsInUseByWindowServer()
    const {
  // IOSurfaceIsInUse() will always return true if the IOSurface is wrapped in
  // a CVPixelBuffer. Ignore the signal for such IOSurfaces (which are the
  // ones output by hardware video decode and video capture).
  if (backing()->usage().Has(SHARED_IMAGE_USAGE_MACOS_VIDEO_TOOLBOX)) {
    return false;
  }

  return IOSurfaceIsInUse(io_surface_.get());
}

// DawnRepresentation and SkiaGraphiteDawnMetalRepresentation are gone with
// Dawn.

///////////////////////////////////////////////////////////////////////////////
// IOSurfaceImageBacking

IOSurfaceImageBacking::IOSurfaceImageBacking(
    gfx::ScopedIOSurface io_surface,
    const Mailbox& mailbox,
    const SharedImageInfo& si_info,
    GLenum gl_target,
    bool framebuffer_attachment_angle,
    bool is_cleared,
    bool is_thread_safe,
    GrContextType gr_context_type,
    std::optional<gfx::BufferUsage> buffer_usage)
    : ClearTrackingSharedImageBacking(
          mailbox,
          si_info,
          si_info.format.EstimatedSizeInBytes(si_info.size),
          is_thread_safe,
          std::move(buffer_usage)),
      io_surface_(std::move(io_surface)),
      io_surface_size_(IOSurfaceGetWidth(io_surface_.get()),
                       IOSurfaceGetHeight(io_surface_.get())),
      io_surface_format_(IOSurfaceGetPixelFormat(io_surface_.get())),
      gl_target_(gl_target),
      framebuffer_attachment_angle_(framebuffer_attachment_angle),
      weak_factory_(this) {
  CHECK(io_surface_);

  // Set the color space for the underlying IOSurface when it's used as overlay.
  gfx::IOSurfaceSetColorSpace(io_surface_.get(), si_info.color_space);

  // If this will be bound to different GL backends, then make RetainGLTexture
  // and ReleaseGLTexture actually create and destroy the texture.
  // https://crbug.com/1251724
  if (si_info.usage.Has(SHARED_IMAGE_USAGE_HIGH_PERFORMANCE_GPU)) {
    return;
  }

  SetClearedRectInternal((is_cleared ? gfx::Rect(si_info.size) : gfx::Rect()));

  // NOTE: Mac currently retains GLTexture and reuses it. This might lead to
  // issues with context losses, but is also beneficial to performance at
  // least on perf benchmarks.
  if (gr_context_type == GrContextType::kGL) {
    // NOTE: We do not CHECK here that the current GL context is that of the
    // SharedContextState due to not having easy access to the
    // SharedContextState here. However, all codepaths that create SharedImage
    // backings make the SharedContextState's context current before doing so.
    egl_state_for_skia_gl_context_ = RetainGLTexture();
  }
}

IOSurfaceImageBacking::~IOSurfaceImageBacking() {
  AutoLock auto_lock(this);
  if (egl_state_for_skia_gl_context_) {
    egl_state_for_skia_gl_context_->WillRelease(have_context());

    if (egl_state_for_skia_gl_context_->created_task_runner()
            ->BelongsToCurrentThread()) {
      egl_state_for_skia_gl_context_ = nullptr;
    } else {
      // Remove `egl_state` from `egl_state_map_`.
      IOSurfaceBackingEGLState* egl_state =
          egl_state_for_skia_gl_context_.get();
      auto key = std::make_pair(egl_state->egl_display_,
                                egl_state->created_task_runner());
      auto found = egl_state_map_.find(key);
      CHECK(found != egl_state_map_.end());
      CHECK(found->second == egl_state);
      egl_state_map_.erase(found);

      // Send egl_state to the original thread for delete. Making
      // the original context current can only be done on the same thread.
      egl_state_for_skia_gl_context_->RemoveClient();
      base::SingleThreadTaskRunner* task_runner =
          egl_state_for_skia_gl_context_->created_task_runner_.get();
      task_runner->PostTask(FROM_HERE, base::DoNothingWithBoundArgs(std::move(
                                           egl_state_for_skia_gl_context_)));
    }
  }
  DCHECK(egl_state_map_.empty());
}

bool IOSurfaceImageBacking::ReadbackToMemory(
    const std::vector<SkPixmap>& pixmaps) {
  AutoLock auto_lock(this);
  CHECK_LE(pixmaps.size(), 3u);

  // Make sure any pending ANGLE EGLDisplays and Dawn devices are flushed.
  WaitForCommandsToBeScheduled();

  ScopedIOSurfaceLock io_surface_lock(io_surface_.get(),
                                      kIOSurfaceLockReadOnly);

  for (int plane_index = 0; plane_index < static_cast<int>(pixmaps.size());
       ++plane_index) {
    const gfx::Size plane_size = format().GetPlaneSize(plane_index, size());

    const void* io_surface_base_address =
        IOSurfaceGetBaseAddressOfPlane(io_surface_.get(), plane_index);
    CHECK_EQ(plane_size.width(), static_cast<int>(IOSurfaceGetWidthOfPlane(
                                     io_surface_.get(), plane_index)));
    CHECK_EQ(plane_size.height(), static_cast<int>(IOSurfaceGetHeightOfPlane(
                                      io_surface_.get(), plane_index)));

    int io_surface_row_bytes = 0;
    int dst_bytes_per_row = 0;

    base::CheckedNumeric<int> checked_io_surface_row_bytes =
        IOSurfaceGetBytesPerRowOfPlane(io_surface_.get(), plane_index);
    base::CheckedNumeric<int> checked_dst_bytes_per_row =
        pixmaps[plane_index].rowBytes();

    if (!checked_io_surface_row_bytes.AssignIfValid(&io_surface_row_bytes) ||
        !checked_dst_bytes_per_row.AssignIfValid(&dst_bytes_per_row)) {
      return false;
    }

    const uint8_t* src_ptr =
        static_cast<const uint8_t*>(io_surface_base_address);
    uint8_t* dst_ptr =
        static_cast<uint8_t*>(pixmaps[plane_index].writable_addr());

    const int copy_bytes =
        static_cast<int>(pixmaps[plane_index].info().minRowBytes());
    CHECK_LE(copy_bytes, io_surface_row_bytes);
    CHECK_LE(copy_bytes, dst_bytes_per_row);

    CopyImagePlane(src_ptr, io_surface_row_bytes, dst_ptr, dst_bytes_per_row,
                   copy_bytes, plane_size.height());
  }

  return true;
}

bool IOSurfaceImageBacking::UploadFromMemory(
    const std::vector<SkPixmap>& pixmaps) {
  AutoLock auto_lock(this);
  CHECK_LE(pixmaps.size(), 3u);

  // Make sure any pending ANGLE EGLDisplays and Dawn devices are flushed.
  WaitForCommandsToBeScheduled();

  ScopedIOSurfaceLock io_surface_lock(io_surface_.get(), /*options=*/0);

  for (int plane_index = 0; plane_index < static_cast<int>(pixmaps.size());
       ++plane_index) {
    const gfx::Size plane_size = format().GetPlaneSize(plane_index, size());

    void* io_surface_base_address =
        IOSurfaceGetBaseAddressOfPlane(io_surface_.get(), plane_index);
    CHECK_EQ(plane_size.width(), static_cast<int>(IOSurfaceGetWidthOfPlane(
                                     io_surface_.get(), plane_index)));
    CHECK_EQ(plane_size.height(), static_cast<int>(IOSurfaceGetHeightOfPlane(
                                      io_surface_.get(), plane_index)));

    int io_surface_row_bytes = 0;
    int src_bytes_per_row = 0;

    base::CheckedNumeric<int> checked_io_surface_row_bytes =
        IOSurfaceGetBytesPerRowOfPlane(io_surface_.get(), plane_index);
    base::CheckedNumeric<int> checked_src_bytes_per_row =
        pixmaps[plane_index].rowBytes();

    if (!checked_io_surface_row_bytes.AssignIfValid(&io_surface_row_bytes) ||
        !checked_src_bytes_per_row.AssignIfValid(&src_bytes_per_row)) {
      return false;
    }

    const uint8_t* src_ptr =
        static_cast<const uint8_t*>(pixmaps[plane_index].addr());

    const int copy_bytes =
        static_cast<int>(pixmaps[plane_index].info().minRowBytes());
    CHECK_LE(copy_bytes, src_bytes_per_row);
    CHECK_LE(copy_bytes, io_surface_row_bytes);

    uint8_t* dst_ptr = static_cast<uint8_t*>(io_surface_base_address);

    CopyImagePlane(src_ptr, src_bytes_per_row, dst_ptr, io_surface_row_bytes,
                   copy_bytes, plane_size.height());
  }

  return true;
}

scoped_refptr<IOSurfaceBackingEGLState>
IOSurfaceImageBacking::RetainGLTexture() {
  AutoLock auto_lock(this);

  gl::GLContext* context = gl::GLContext::GetCurrent();
  gl::GLDisplayEGL* display = context ? context->GetGLDisplayEGL() : nullptr;
  if (!display) {
    LOG(ERROR) << "No GLDisplayEGL current.";
    return nullptr;
  }
  const EGLDisplay egl_display = display->GetDisplay();

  auto key = std::make_pair(egl_display,
                            base::SingleThreadTaskRunner::GetCurrentDefault());
  auto found = egl_state_map_.find(key);
  if (found != egl_state_map_.end())
    return found->second;

  std::vector<scoped_refptr<gles2::TexturePassthrough>> gl_textures;
  for (int plane_index = 0; plane_index < format().NumberOfPlanes();
       plane_index++) {
    // Allocate the GL texture.
    scoped_refptr<gles2::TexturePassthrough> gl_texture;
    MakeTextureAndSetParameters(gl_target_, framebuffer_attachment_angle_,
                                &gl_texture, nullptr);
    // Set the IOSurface to be initially unbound from the GL texture.
    gl_texture->SetEstimatedSize(format().EstimatedSizeInBytes(size()));

    gl_textures.push_back(std::move(gl_texture));
  }

  scoped_refptr<IOSurfaceBackingEGLState> egl_state =
      new IOSurfaceBackingEGLState(this, egl_display, context,
                                   gl::GLSurface::GetCurrent(), gl_target_,
                                   std::move(gl_textures));
  egl_state->set_bind_pending();
  return egl_state;
}

void IOSurfaceImageBacking::ReleaseGLTexture(
    IOSurfaceBackingEGLState* egl_state,
    bool have_context) {
  AssertLockAcquired();
  DCHECK_EQ(static_cast<int>(egl_state->gl_textures_.size()),
            format().NumberOfPlanes());
  DCHECK(egl_state->egl_surfaces_.empty() ||
         static_cast<int>(egl_state->egl_surfaces_.size()) ==
             format().NumberOfPlanes());

  if (!have_context) {
    for (const auto& texture : egl_state->gl_textures_) {
      texture->MarkContextLost();
    }
  }
  egl_state->gl_textures_.clear();
}

base::trace_event::MemoryAllocatorDump* IOSurfaceImageBacking::OnMemoryDump(
    const std::string& dump_name,
    base::trace_event::MemoryAllocatorDumpGuid client_guid,
    base::trace_event::ProcessMemoryDump* pmd,
    uint64_t client_tracing_id) {
  auto* dump = SharedImageBacking::OnMemoryDump(dump_name, client_guid, pmd,
                                                client_tracing_id);

  size_t size_bytes = 0u;
  for (int plane = 0; plane < format().NumberOfPlanes(); plane++) {
    size_bytes += IOSurfaceGetBytesPerRowOfPlane(io_surface_.get(), plane) *
                  IOSurfaceGetHeightOfPlane(io_surface_.get(), plane);
  }

  dump->AddScalar(base::trace_event::MemoryAllocatorDump::kNameSize,
                  base::trace_event::MemoryAllocatorDump::kUnitsBytes,
                  static_cast<uint64_t>(size_bytes));

  return dump;
}

SharedImageBackingType IOSurfaceImageBacking::GetType() const {
  return SharedImageBackingType::kIOSurface;
}

std::unique_ptr<GLTextureImageRepresentation>
IOSurfaceImageBacking::ProduceGLTexture(SharedImageManager* manager,
                                        MemoryTypeTracker* tracker) {
  return nullptr;
}

std::unique_ptr<GLTexturePassthroughImageRepresentation>
IOSurfaceImageBacking::ProduceGLTexturePassthrough(SharedImageManager* manager,
                                                   MemoryTypeTracker* tracker) {
  scoped_refptr<IOSurfaceBackingEGLState> egl_state;
  egl_state = RetainGLTexture();

  // The corresponding release will be done when the returned representation is
  // destroyed, in GLTextureImageRepresentationBeingDestroyed.
  return std::make_unique<GLTextureIRepresentation>(
      manager, this, std::move(egl_state), tracker);
}

std::unique_ptr<OverlayImageRepresentation>
IOSurfaceImageBacking::ProduceOverlay(SharedImageManager* manager,
                                      MemoryTypeTracker* tracker) {
  return std::make_unique<OverlayRepresentation>(manager, this, tracker,
                                                 io_surface_);
}



void IOSurfaceImageBacking::WaitForCommandsToBeScheduled(
    id<MTLDevice> waiting_device) {
  AssertLockAcquired();
  TRACE_EVENT0("gpu", "IOSurfaceImageBacking::WaitForCommandsToBeScheduled");

  // This used to also wait on Dawn commands-scheduled futures here; gone
  // with Dawn.

  base::flat_map<EGLDisplay, std::unique_ptr<gl::GLFenceEGL>> fences_to_keep;
  for (auto& [display, fence] : egl_commands_scheduled_fences_) {
    if (QueryMetalDeviceFromANGLE(display) == waiting_device) {
      fences_to_keep.emplace(display, std::move(fence));
      continue;
    }

    TRACE_EVENT0("gpu",
                 "IOSurfaceImageBacking::WaitForCommandsToBeScheduled::ANGLE");
    fence->ClientWait();
  }
  egl_commands_scheduled_fences_ = std::move(fences_to_keep);
}

IOSurfaceRef IOSurfaceImageBacking::GetIOSurface() {
  return io_surface_.get();
}


std::unique_ptr<SkiaGaneshImageRepresentation>
IOSurfaceImageBacking::ProduceSkiaGanesh(
    SharedImageManager* manager,
    MemoryTypeTracker* tracker,
    scoped_refptr<SharedContextState> context_state) {
  scoped_refptr<IOSurfaceBackingEGLState> egl_state;
  std::vector<sk_sp<GrPromiseImageTexture>> promise_textures;

  if (context_state->GrContextIsGL()) {
    egl_state = RetainGLTexture();
  }

  {
    AutoLock auto_lock(this);
    for (int plane_index = 0; plane_index < format().NumberOfPlanes();
         plane_index++) {
      GLFormatDesc format_desc =
          context_state->GetGLFormatCaps().ToGLFormatDesc(format(),
                                                          plane_index);
      GrBackendTexture backend_texture;
      auto plane_size = format().GetPlaneSize(plane_index, size());
      GetGrBackendTexture(
          context_state->feature_info(), egl_state->GetGLTarget(), plane_size,
          egl_state->GetGLServiceId(plane_index),
          format_desc.storage_internal_format,
          context_state->gr_context()->threadSafeProxy(), &backend_texture);
      sk_sp<GrPromiseImageTexture> promise_texture =
          GrPromiseImageTexture::Make(backend_texture);
      if (!promise_texture) {
        return nullptr;
      }
      promise_textures.push_back(std::move(promise_texture));
    }
  }

  return std::make_unique<SkiaGaneshRepresentation>(manager, this, egl_state,
                                                    std::move(context_state),
                                                    promise_textures, tracker);
}

std::unique_ptr<SkiaGraphiteImageRepresentation>
IOSurfaceImageBacking::ProduceSkiaGraphite(
    SharedImageManager* manager,
    MemoryTypeTracker* tracker,
    scoped_refptr<SharedContextState> context_state) {
  // This used to wrap the IOSurface as a Dawn texture via
  // context_state->dawn_context_provider(). Dawn is gone, and
  // IsGraphiteDawn() is now always false, so there is no Graphite backend
  // left for this to produce a representation for.
  return nullptr;
}

void IOSurfaceImageBacking::SetPurgeable(bool purgeable) {
  AutoLock auto_lock(this);
  if (purgeable_ == purgeable)
    return;
  purgeable_ = purgeable;

  if (purgeable) {
    // It is in error to purge the surface while reading or writing to it.
    DCHECK(!ongoing_write_access_);
    DCHECK(!num_ongoing_read_accesses_);

    SetClearedRectInternal(gfx::Rect());
  }

  uint32_t old_state;
  IOSurfaceSetPurgeable(io_surface_.get(), purgeable, &old_state);
}

bool IOSurfaceImageBacking::IsPurgeable() const {
  AutoLock auto_lock(this);
  return purgeable_;
}

void IOSurfaceImageBacking::Update(std::unique_ptr<gfx::GpuFence> in_fence) {
  AutoLock auto_lock(this);
#if BUILDFLAG(IS_IOS)
  {
    // On iOS, we can't use IOKit to access IOSurfaces in the renderer process,
    // so we share the memory segment backing the IOSurface as shared memory
    // which is then mapped in the renderer process. We need to signal that the
    // IOSurface was updated on the CPU so we do an IOSurfaceLock+Unlock here in
    // case there are other consumers of the IOSurface that rely on its internal
    // seed value to detect updates - the lock+unlock updates the seed value.
    // TODO(crbug.com/40254930): Assert that we have CPU_WRITE_ONLY usage so
    // that we never have the client's CPU-written data overwritten due to a
    // shadow copy from the GPU - we can also use kIOSurfaceLockAvoidSync then.
    ScopedIOSurfaceLock io_surface_lock(io_surface_.get(), /*options=*/0);
  }
#endif
  for (auto iter : egl_state_map_) {
    iter.second->set_bind_pending();
  }
}

gfx::GpuMemoryBufferHandle IOSurfaceImageBacking::GetGpuMemoryBufferHandle() {
  return gfx::GpuMemoryBufferHandle(io_surface_);
}


bool IOSurfaceImageBacking::BeginAccessWebNN() {
  AutoLock auto_lock(this);
  if (!BeginAccess(/*readonly=*/false)) {
    return false;
  }
  WaitForCommandsToBeScheduled();
  return true;
}

void IOSurfaceImageBacking::EndAccessWebNN() {
  AutoLock auto_lock(this);

  EndAccess(/*readonly=*/false);
}


bool IOSurfaceImageBacking::BeginAccess(bool readonly) {
  AssertLockAcquired();

  CHECK_GE(num_ongoing_read_accesses_, 0);

  if (!readonly && ongoing_write_access_) {
    DLOG(ERROR) << "Unable to begin write access because another "
                   "write access is in progress";
    return false;
  }
  // Track reads and writes if not being used for concurrent read/writes.
  if (!(usage().Has(SHARED_IMAGE_USAGE_CONCURRENT_READ_WRITE))) {
    if (readonly && ongoing_write_access_) {
      DLOG(ERROR) << "Unable to begin read access because another "
                     "write access is in progress";
      return false;
    }
    if (!readonly && num_ongoing_read_accesses_ > 0) {
      DLOG(ERROR) << "Unable to begin write access because a read access is in "
                     "progress";
      return false;
    }
  }

  if (readonly) {
    num_ongoing_read_accesses_++;
  } else {
    ongoing_write_access_ = true;
  }

  return true;
}

void IOSurfaceImageBacking::EndAccess(bool readonly) {
  AssertLockAcquired();

  if (readonly) {
    CHECK_GT(num_ongoing_read_accesses_, 0);
    if (!(usage().Has(SHARED_IMAGE_USAGE_CONCURRENT_READ_WRITE))) {
      CHECK(!ongoing_write_access_);
    }
    num_ongoing_read_accesses_--;
  } else {
    CHECK(ongoing_write_access_);
    if (!(usage().Has(SHARED_IMAGE_USAGE_CONCURRENT_READ_WRITE))) {
      CHECK_EQ(num_ongoing_read_accesses_, 0);
    }
    ongoing_write_access_ = false;
  }
}

bool IOSurfaceImageBacking::IOSurfaceBackingEGLStateBeginAccess(
    IOSurfaceBackingEGLState* egl_state,
    bool readonly) {
  AssertLockAcquired();

  // It is in error to read or write an IOSurface while it is purgeable.
  CHECK(!purgeable_);
  if (!BeginAccess(readonly)) {
    return false;
  }

  CHECK_GE(egl_state->num_ongoing_accesses_, 0);

  gl::GLDisplayEGL* display = gl::GLDisplayEGL::GetDisplayForCurrentContext();
  CHECK(display);

  EGLDisplay egl_display = display->GetDisplay();
  CHECK_EQ(egl_display, egl_state->egl_display_);

  // Note that we don't need to wait for commands to be scheduled for other
  // EGLDisplays because it is already done when the previous GL context is made
  // uncurrent. We can simply remove fences for the other EGLDisplays.
  base::EraseIf(egl_commands_scheduled_fences_, [egl_display](const auto& kv) {
    return kv.first != egl_display;
  });

  // IOSurface might be written on a different queue. So we have to wait for the
  // previous Dawn and ANGLE commands to be scheduled first so that the kernel
  // knows about the pending update to the IOSurface.
  WaitForCommandsToBeScheduled(QueryMetalDeviceFromANGLE(egl_display));

  if (gl::GetANGLEImplementation() == gl::ANGLEImplementation::kMetal) {
    // If this image could potentially be shared with another Metal device,
    // it's necessary to synchronize between the two devices. If any Metal
    // shared events have been enqueued (the assumption is that this was done by
    // for a Dawn device or another ANGLE Metal EGLDisplay), wait on them.
    ProcessSharedEventsForBeginAccess(
        readonly,
        [display](id<MTLSharedEvent> shared_event, uint64_t signaled_value) {
          display->WaitForMetalSharedEvent(shared_event, signaled_value);
        });
  }

  // If the GL texture is already bound (the bind is not marked as pending),
  // then early-out.
  if (!egl_state->is_bind_pending()) {
    CHECK(!egl_state->egl_surfaces_.empty());
    egl_state->num_ongoing_accesses_++;
    return true;
  }

  if (egl_state->egl_surfaces_.empty()) {
    std::vector<std::unique_ptr<gl::ScopedEGLSurfaceIOSurface>> egl_surfaces;
    for (int plane_index = 0; plane_index < format().NumberOfPlanes();
         plane_index++) {
      viz::SharedImageFormat plane_format;
      if (format().is_single_plane()) {
        plane_format = format();
        // See comments in IOSurfaceImageBackingFactory::CreateSharedImage about
        // RGBA versus BGRA when using Skia Ganesh GL backend or ANGLE.
        if (io_surface_format_ == 'BGRA') {
          if (plane_format == viz::SinglePlaneFormat::kRGBA_8888) {
            plane_format = viz::SinglePlaneFormat::kBGRA_8888;
          } else if (plane_format == viz::SinglePlaneFormat::kRGBX_8888) {
            plane_format = viz::SinglePlaneFormat::kBGRX_8888;
          }
        }
      } else {
        // For multiplanar formats (without external sampler) get planar buffer
        // format.
        plane_format = GetFormatForPlane(format(), plane_index);
      }

      auto egl_surface = gl::ScopedEGLSurfaceIOSurface::Create(
          egl_state->egl_display_, egl_state->GetGLTarget(), io_surface_.get(),
          plane_index, plane_format);
      if (!egl_surface) {
        LOG(ERROR) << "Failed to create ScopedEGLSurfaceIOSurface.";
        EndAccess(readonly);
        return false;
      }

      egl_surfaces.push_back(std::move(egl_surface));
    }
    egl_state->egl_surfaces_ = std::move(egl_surfaces);
  }

  CHECK_EQ(static_cast<int>(egl_state->gl_textures_.size()),
           format().NumberOfPlanes());
  CHECK_EQ(static_cast<int>(egl_state->egl_surfaces_.size()),
           format().NumberOfPlanes());
  for (int plane_index = 0; plane_index < format().NumberOfPlanes();
       plane_index++) {
    gl::ScopedRestoreTexture scoped_restore(
        gl::g_current_gl_context, egl_state->GetGLTarget(),
        egl_state->GetGLServiceId(plane_index));
    // Un-bind the IOSurface from the GL texture (this will be a no-op if it is
    // not yet bound).
    egl_state->egl_surfaces_[plane_index]->ReleaseTexImage();

    // Bind the IOSurface to the GL texture.
    if (!egl_state->egl_surfaces_[plane_index]->BindTexImage()) {
      LOG(ERROR) << "Failed to bind ScopedEGLSurfaceIOSurface to target.";
      EndAccess(readonly);
      return false;
    }
  }
  egl_state->clear_bind_pending();
  egl_state->num_ongoing_accesses_++;

  return true;
}

void IOSurfaceImageBacking::IOSurfaceBackingEGLStateEndAccess(
    IOSurfaceBackingEGLState* egl_state,
    bool readonly) {
  AssertLockAcquired();

  // Early out if BeginAccess didn't succeed and we didn't bind any surfaces.
  if (egl_state->is_bind_pending()) {
    return;
  }

  CHECK_GT(egl_state->num_ongoing_accesses_, 0);
  egl_state->num_ongoing_accesses_--;

  EndAccess(readonly);

  gl::GLDisplayEGL* display = gl::GLDisplayEGL::GetDisplayForCurrentContext();
  CHECK(display);

  EGLDisplay egl_display = display->GetDisplay();
  CHECK_EQ(egl_display, egl_state->egl_display_);

  const bool is_angle_metal =
      gl::GetANGLEImplementation() == gl::ANGLEImplementation::kMetal;
  if (is_angle_metal) {
    id<MTLSharedEvent> shared_event = nil;
    uint64_t signal_value = 0;
    if (display->CreateMetalSharedEvent(&shared_event, &signal_value)) {
      AddSharedEventForEndAccess(shared_event, signal_value, readonly);
    } else {
      LOG(DFATAL) << "Failed to create Metal shared event";
    }
  }

  // When SwANGLE is used as the GL implementation, it holds an internal
  // texture. We have to call ReleaseTexImage here to trigger a copy from that
  // internal texture to the IOSurface (the next Bind() will then trigger an
  // IOSurface->internal texture copy). We do this only when there are no
  // ongoing reads in order to ensure that it does not result in the GLES2
  // decoders needing to perform on-demand binding (rather, the binding will be
  // performed at the next BeginAccess()). Note that it is not sufficient to
  // release the image only at the end of a write: the CPU can write directly to
  // the IOSurface when the GPU is not accessing the internal texture (in the
  // case of zero-copy raster), and any such IOSurface-side modifications need
  // to be copied to the internal texture via a Bind() when the GPU starts a
  // subsequent read. Note also that this logic assumes that writes are
  // serialized with respect to reads (so that the end of a write always
  // triggers a release and copy). By design, IOSurfaceImageBackingFactory
  // enforces this property for this use case.
  //
  // For ANGLE Metal, we need to rebind the texture so that the BindTexImage
  // adds a synchronization dependency on the command buffer which contains the
  // shared event wait before the next ANGLE access. Otherwise, ANGLE might skip
  // waiting on the command buffer and hence the shared event and do a CPU
  // readback from the IOSurface in some cases without synchronization.
  const bool is_swangle =
      gl::GetANGLEImplementation() == gl::ANGLEImplementation::kSwiftShader;
  if ((is_swangle || is_angle_metal) && egl_state->num_ongoing_accesses_ == 0) {
    CHECK_EQ(static_cast<int>(egl_state->gl_textures_.size()),
             format().NumberOfPlanes());
    CHECK_EQ(static_cast<int>(egl_state->egl_surfaces_.size()),
             format().NumberOfPlanes());
    for (int plane_index = 0; plane_index < format().NumberOfPlanes();
         plane_index++) {
      gl::ScopedRestoreTexture scoped_restore(
          gl::g_current_gl_context, egl_state->GetGLTarget(),
          egl_state->GetGLServiceId(plane_index));
      egl_state->egl_surfaces_[plane_index]->ReleaseTexImage();
    }
    egl_state->set_bind_pending();
  }

  // We have to wait for pending work to be scheduled on the GPU for IOSurface
  // synchronization by the kernel e.g. using waitUntilScheduled on Metal or
  // glFlush on OpenGL.
  if (is_angle_metal) {
    // Defer the wait until CoreAnimation, Dawn, or another ANGLE EGLDisplay
    // needs to access to avoid unnecessary overhead. This also ensures that the
    // Metal shared event signal which is enqueued above is flushed.
    auto fence = gl::GLFenceEGL::Create(EGL_SYNC_METAL_COMMANDS_SCHEDULED_ANGLE,
                                        nullptr);
    if (fence) {
      egl_commands_scheduled_fences_.insert_or_assign(egl_display,
                                                      std::move(fence));
    } else {
      LOG(ERROR)
          << "Failed to create EGL_SYNC_METAL_COMMANDS_SCHEDULED_ANGLE fence";
    }
  } else {
    eglWaitUntilWorkScheduledANGLE(egl_display);
  }
}

void IOSurfaceImageBacking::IOSurfaceBackingEGLStateBeingCreated(
    IOSurfaceBackingEGLState* egl_state) {
  AssertLockAcquired();

  auto key =
      std::make_pair(egl_state->egl_display_, egl_state->created_task_runner());
  auto insert_result = egl_state_map_.insert(std::make_pair(key, egl_state));
  CHECK(insert_result.second);
}

void IOSurfaceImageBacking::IOSurfaceBackingEGLStateBeingDestroyed(
    IOSurfaceBackingEGLState* egl_state,
    bool has_context) {
  AssertLockAcquired();
  ReleaseGLTexture(egl_state, has_context);

  egl_state->egl_surfaces_.clear();

  // Remove `egl_state` from `egl_state_map_`.
  auto key =
      std::make_pair(egl_state->egl_display_, egl_state->created_task_runner());
  auto found = egl_state_map_.find(key);
  CHECK(found != egl_state_map_.end());
  CHECK(found->second == egl_state);
  egl_state_map_.erase(found);
}

bool IOSurfaceImageBacking::InitializePixels(
    base::span<const uint8_t> pixel_data) {
  AutoLock auto_lock(this);
  CHECK(format().is_single_plane());
  ScopedIOSurfaceLock io_surface_lock(io_surface_.get(),
                                      kIOSurfaceLockAvoidSync);

  uint8_t* dst_data = reinterpret_cast<uint8_t*>(
      IOSurfaceGetBaseAddressOfPlane(io_surface_.get(), 0));
  size_t dst_stride = IOSurfaceGetBytesPerRowOfPlane(io_surface_.get(), 0);

  const uint8_t* src_data = pixel_data.data();
  const size_t src_stride = format().BytesPerPixel() * size().width();
  const size_t height = size().height();

  if (pixel_data.size() != src_stride * height) {
    DLOG(ERROR) << "Invalid initial pixel data size";
    return false;
  }

  for (size_t y = 0; y < height; ++y) {
    UNSAFE_TODO(memcpy(dst_data, src_data, src_stride));
    UNSAFE_TODO(dst_data += dst_stride);
    UNSAFE_TODO(src_data += src_stride);
  }

  return true;
}

void IOSurfaceImageBacking::AddSharedEventForEndAccess(
    id<MTLSharedEvent> shared_event,
    uint64_t signal_value,
    bool readonly) {
  AssertLockAcquired();

  SharedEventMap& shared_events =
      readonly ? non_exclusive_shared_events_ : exclusive_shared_events_;
  auto [it, _] = shared_events.insert(
      {ScopedSharedEvent(shared_event, base::scoped_policy::RETAIN), 0});
  it->second = std::max(it->second, signal_value);
}

void IOSurfaceImageBacking::ProcessSharedEventsForBeginAccess(
    bool readonly,
    base::FunctionRef<void(id<MTLSharedEvent> shared_event,
                           uint64_t signaled_value)> process_fn) {
  AssertLockAcquired();

  // Always need wait on exclusive access end events.
  for (const auto& [shared_event, signal_value] : exclusive_shared_events_) {
    process_fn(shared_event.get(), signal_value);
  }

  if (!readonly) {
    // For read-write (exclusive) access, non execlusive access end events
    // should be waited on as well.
    for (const auto& [shared_event, signal_value] :
         non_exclusive_shared_events_) {
      process_fn(shared_event.get(), signal_value);
    }

    // Clear events, since this read-write (exclusive) access will provide an
    // event when the access is finished.
    exclusive_shared_events_.clear();
    non_exclusive_shared_events_.clear();
  }
}

}  // namespace gpu
