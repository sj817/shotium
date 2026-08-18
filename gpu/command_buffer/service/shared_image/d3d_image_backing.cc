// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image/d3d_image_backing.h"

#include <d3d11_3.h>
#include <d3d11on12.h>
#include <wrl/client.h>

#include "base/compiler_specific.h"

// clang-format off
// clang-format on

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/ptr_util.h"
#include "base/notreached.h"
#include "base/strings/stringprintf.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/single_thread_task_runner.h"
#include "base/trace_event/trace_event.h"
#include "gpu/command_buffer/common/shared_image_trace_utils.h"
#include "gpu/command_buffer/common/shared_image_usage.h"
#include "gpu/command_buffer/service/dxgi_shared_handle_manager.h"
#include "gpu/command_buffer/service/shared_context_state.h"
#include "gpu/command_buffer/service/shared_image/copy_image_plane.h"
#include "gpu/command_buffer/service/shared_image/d3d_image_representation.h"
#include "gpu/command_buffer/service/shared_image/d3d_image_utils.h"
#include "gpu/command_buffer/service/shared_image/shared_image_format_service_utils.h"
#include "gpu/command_buffer/service/shared_image/skia_gl_image_representation.h"
#include "gpu/config/gpu_finch_features.h"
#include "third_party/skia/include/gpu/ganesh/GrBackendSemaphore.h"
#include "ui/gfx/color_space_win.h"
#include "ui/gl/direct_composition_support.h"
#include "ui/gl/egl_util.h"
#include "ui/gl/gl_angle_util_win.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_features.h"
#include "ui/gl/scoped_restore_texture.h"

#ifndef EGL_ANGLE_image_d3d11_texture
#define EGL_D3D11_TEXTURE_ANGLE 0x3484
#define EGL_TEXTURE_INTERNAL_FORMAT_ANGLE 0x345D
#define EGL_D3D11_TEXTURE_PLANE_ANGLE 0x3492
#define EGL_D3D11_TEXTURE_ARRAY_SLICE_ANGLE 0x3493
#endif /* EGL_ANGLE_image_d3d11_texture */

namespace gpu {

namespace {

// Returns true if `d3d11_device` is a D3D11On12 device whose texture can be
// unwrapped to a D3D12 resource. Requires the kDCompOnD3D12 feature.
bool CanUseD3D12(ID3D11Device* d3d11_device,
                 const D3D11_TEXTURE2D_DESC& d3d11_texture_desc) {
  if (!base::FeatureList::IsEnabled(features::kDCompOnD3D12)) {
    return false;
  }
  Microsoft::WRL::ComPtr<ID3D11On12Device2> d3d11on12_device;
  HRESULT hr = d3d11_device->QueryInterface(IID_PPV_ARGS(&d3d11on12_device));
  // D3D11 keyed mutex resources can not be unwrapped to D3D12 resources by
  // D3D11On12.
  return SUCCEEDED(hr) && !(d3d11_texture_desc.MiscFlags &
                            D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX);
}

// Unwraps a D3D11on12 texture to its underlying D3D12 resource. Note that
// UnwrapUnderlyingResource is not guaranteed to return the same D3D12 resource
// for multiple calls with the same D3D11 resource.
Microsoft::WRL::ComPtr<ID3D12Resource> PrepareD3D11on12TextureForD3D12(
    ID3D11Resource* d3d11_texture,
    ID3D12CommandQueue* d3d12_command_queue) {
  CHECK(d3d11_texture);
  CHECK(d3d12_command_queue);
  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
  d3d11_texture->GetDevice(&d3d11_device);
  Microsoft::WRL::ComPtr<ID3D11On12Device2> d3d11on12_device;
  HRESULT hr = d3d11_device.As(&d3d11on12_device);
  CHECK_EQ(hr, S_OK);

  // Get D3D12 resource ready.
  Microsoft::WRL::ComPtr<ID3D12Resource> d3d12_resource;
  hr = d3d11on12_device->UnwrapUnderlyingResource(
      d3d11_texture, d3d12_command_queue, IID_PPV_ARGS(&d3d12_resource));
  CHECK_EQ(hr, S_OK) << "Failed to unwrap D3D11 texture: "
                     << logging::SystemErrorCodeToString(hr);

  // UnwrapUnderlyingResource doesn't flush, and it may schedule GPU work. We
  // should therefore flush the D3D11 device context here. See:
  // https://learn.microsoft.com/en-us/windows/win32/api/d3d11on12/nf-d3d11on12-id3d11on12device2-unwrapunderlyingresource
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
  d3d11_device->GetImmediateContext(&device_context);
  device_context->Flush();

  return d3d12_resource;
}

// Returns the D3D11on12 texture back to the 11On12 layer.
void PrepareD3D11on12TextureForD3D11(ID3D11Resource* d3d11_texture) {
  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
  d3d11_texture->GetDevice(&d3d11_device);
  Microsoft::WRL::ComPtr<ID3D11On12Device2> d3d11on12_device2;
  HRESULT hr = d3d11_device.As(&d3d11on12_device2);
  CHECK_EQ(hr, S_OK);

  // Acquire wrapped resources.
  d3d11on12_device2->ReturnUnderlyingResource(d3d11_texture, 0, nullptr,
                                              nullptr);
}

bool BindEGLImageToTexture(GLenum texture_target, void* egl_image) {
  if (!egl_image) {
    LOG(ERROR) << "EGL image is null";
    return false;
  }
  glEGLImageTargetTexture2DOES(texture_target, egl_image);
  if (eglGetError() != static_cast<EGLint>(EGL_SUCCESS)) {
    LOG(ERROR) << "Failed to bind EGL image to the texture"
               << ui::GetLastEGLErrorString();
    return false;
  }
  return true;
}

bool CanUseUpdateSubresource(const std::vector<SkPixmap>& pixmaps) {
  if (pixmaps.size() == 1u) {
    return true;
  }

  const uint8_t* addr = static_cast<const uint8_t*>(pixmaps[0].addr());
  size_t plane_offset = pixmaps[0].computeByteSize();
  for (size_t i = 1; i < pixmaps.size(); ++i) {
    // UpdateSubresource() cannot update planes individually, so the planes'
    // data has to be packed in one memory block.
    if (static_cast<const uint8_t*>(pixmaps[i].addr()) !=
        UNSAFE_TODO(addr + plane_offset)) {
      return false;
    }
    plane_offset += pixmaps[i].computeByteSize();
  }

  return true;
}


Microsoft::WRL::ComPtr<IDCompositionTexture> CreateDCompTexture(
    IUnknown* d3d_resource,
    SkAlphaType alpha_type,
    const gfx::ColorSpace& color_space) {
  HRESULT hr = S_OK;

  Microsoft::WRL::ComPtr<IDCompositionDevice3> dcomp_device =
      gl::GetDirectCompositionDevice();
  Microsoft::WRL::ComPtr<IDCompositionDevice4> dcomp_device4;
  hr = dcomp_device.As(&dcomp_device4);
  CHECK_EQ(hr, S_OK) << ", QueryInterface failed: "
                     << logging::SystemErrorCodeToString(hr);

  Microsoft::WRL::ComPtr<IDCompositionTexture> dcomp_texture;
  hr = dcomp_device4->CreateCompositionTexture(d3d_resource, &dcomp_texture);
  CHECK_EQ(hr, S_OK) << ", CreateCompositionTexture failed: "
                     << logging::SystemErrorCodeToString(hr);

  hr = dcomp_texture->SetAlphaMode(SkAlphaTypeIsOpaque(alpha_type)
                                       ? DXGI_ALPHA_MODE_IGNORE
                                       : DXGI_ALPHA_MODE_PREMULTIPLIED);
  CHECK_EQ(hr, S_OK) << ", SetAlphaMode failed: "
                     << logging::SystemErrorCodeToString(hr);

  hr = dcomp_texture->SetColorSpace(
      gfx::ColorSpaceWin::GetDXGIColorSpace(color_space));
  CHECK_EQ(hr, S_OK) << ", SetColorSpace failed: "
                     << logging::SystemErrorCodeToString(hr);

  return dcomp_texture;
}

// Returns true if we need to wait for the DComp texture fence, false if the
// fence is already past the wait value.
template <typename T>
bool ShouldWaitForDCompTextureFence(T& d3d_fence,
                                    uint64_t& fence_value,
                                    IDCompositionTexture* dcomp_texture) {
  HRESULT hr =
      dcomp_texture->GetAvailableFence(&fence_value, IID_PPV_ARGS(&d3d_fence));
  CHECK_EQ(hr, S_OK) << ", GetAvailableFence failed: "
                     << logging::SystemErrorCodeToString(hr);
  // `GetAvailableFence` will return a null fence if the texture is still
  // attached to the DComp tree. We cannot end the read access at this point
  // since DWM can still scanout from the texture. This is probably a bug where
  // the output device ended an overlay access while the overlay image was
  // still in the DComp tree.
  //
  // This can also trigger if we have multiple concurrent outstanding overlay
  // read accesses, which is not currently supported.
  CHECK(d3d_fence) << "Overlay access is still in use by DWM.";

  // If the fence is already past the wait value, we don't need to wait on it.
  return d3d_fence->GetCompletedValue() < fence_value;
}

}  // namespace

// GLTextureHolder
D3DImageBacking::GLTextureHolder::GLTextureHolder(
    base::PassKey<D3DImageBacking>,
    scoped_refptr<gles2::TexturePassthrough> texture_passthrough,
    gl::ScopedEGLImage egl_image)
    : texture_passthrough_(std::move(texture_passthrough)),
      egl_image_(std::move(egl_image)) {}

bool D3DImageBacking::GLTextureHolder::BindEGLImageToTexture() {
  if (!needs_rebind_) {
    return true;
  }

  gl::GLApi* const api = gl::g_current_gl_context;
  gl::ScopedRestoreTexture scoped_restore(api, GL_TEXTURE_2D);

  DCHECK_EQ(texture_passthrough_->target(),
            static_cast<unsigned>(GL_TEXTURE_2D));
  api->glBindTextureFn(GL_TEXTURE_2D, texture_passthrough_->service_id());

  if (!::gpu::BindEGLImageToTexture(GL_TEXTURE_2D, egl_image_.get())) {
    return false;
  }

  needs_rebind_ = false;
  return true;
}

void D3DImageBacking::GLTextureHolder::MarkContextLost() {
  if (texture_passthrough_) {
    texture_passthrough_->MarkContextLost();
  }
}

D3DImageBacking::GLTextureHolder::~GLTextureHolder() = default;

// The PersistentGraphiteDawnAccess class used to keep a Dawn SharedTextureMemory
// access open indefinitely for Graphite so ANGLE/Dawn could share the D3D11
// texture without repeated Begin/EndAccess calls. Dawn is gone, so this
// backing no longer has a persistent Graphite access; the
// NotifyGraphiteAboutInitializedStatus/FlushGraphiteCommandsIfNeeded/
// InvalidatePersistentGraphiteDawnAccess/SupportsDeferredGraphiteSubmit
// methods below are now no-ops kept only because ProduceSkiaGanesh/other
// callers still call them unconditionally.

// static
scoped_refptr<D3DImageBacking::GLTextureHolder>
D3DImageBacking::CreateGLTexture(
    const GLFormatDesc& gl_format_desc,
    Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture,
    GLenum texture_target,
    unsigned array_slice,
    unsigned plane_index) {
  gl::GLApi* const api = gl::g_current_gl_context;
  gl::ScopedRestoreTexture scoped_restore(api, texture_target);

  GLuint service_id = 0;
  api->glGenTexturesFn(1, &service_id);
  api->glBindTextureFn(texture_target, service_id);

  // These need to be set for the texture to be considered mipmap complete.
  api->glTexParameteriFn(texture_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  api->glTexParameteriFn(texture_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // These are not strictly required but guard against some checks if NPOT
  // texture support is disabled.
  api->glTexParameteriFn(texture_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  api->glTexParameteriFn(texture_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  const EGLint egl_attrib_list[] = {
      EGL_TEXTURE_INTERNAL_FORMAT_ANGLE,
      static_cast<EGLint>(gl_format_desc.image_internal_format),
      EGL_D3D11_TEXTURE_ARRAY_SLICE_ANGLE,
      static_cast<EGLint>(array_slice),
      EGL_D3D11_TEXTURE_PLANE_ANGLE,
      static_cast<EGLint>(plane_index),
      EGL_NONE};

  auto egl_image = gl::MakeScopedEGLImage(
      EGL_NO_CONTEXT, EGL_D3D11_TEXTURE_ANGLE,
      static_cast<EGLClientBuffer>(d3d11_texture.Get()), egl_attrib_list);

  if (!egl_image.get()) {
    LOG(ERROR) << "Failed to create an EGL image";
    api->glDeleteTexturesFn(1, &service_id);
    return nullptr;
  }

  if (!BindEGLImageToTexture(texture_target, egl_image.get())) {
    return nullptr;
  }

  auto texture = base::MakeRefCounted<gles2::TexturePassthrough>(
      service_id, texture_target);
  GLint texture_memory_size = 0;
  api->glGetTexParameterivFn(texture_target, GL_MEMORY_SIZE_ANGLE,
                             &texture_memory_size);
  texture->SetEstimatedSize(texture_memory_size);

  return base::MakeRefCounted<GLTextureHolder>(base::PassKey<D3DImageBacking>(),
                                               std::move(texture),
                                               std::move(egl_image));
}

// static
std::unique_ptr<D3DImageBacking> D3DImageBacking::CreateFromSwapChainBuffers(
    const Mailbox& mailbox,
    viz::SharedImageFormat format,
    const gfx::Size& size,
    const gfx::ColorSpace& color_space,
    GrSurfaceOrigin surface_origin,
    SkAlphaType alpha_type,
    gpu::SharedImageUsageSet usage,
    Microsoft::WRL::ComPtr<ID3D11Texture2D> back_buffer_texture,
    Microsoft::WRL::ComPtr<ID3D11Texture2D> front_buffer_texture,
    Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_chain,
    const GLFormatCaps& gl_format_caps) {
  DCHECK(format.is_single_plane());
  auto backing = base::WrapUnique(new D3DImageBacking(
      mailbox,
      SharedImageInfo(format, size, color_space, surface_origin, alpha_type,
                      usage, "SwapChainBuffer"),
      std::move(back_buffer_texture),
      /*dxgi_shared_handle_state=*/nullptr, gl_format_caps, GL_TEXTURE_2D,
      /*array_slice=*/0u));
  backing->swap_chain_ = std::move(swap_chain);
  backing->swap_chain_front_buffer_texture_ = std::move(front_buffer_texture);
  return backing;
}

// static
std::unique_ptr<D3DImageBacking> D3DImageBacking::CreateFromD3D12Buffer(
    const Mailbox& mailbox,
    const gfx::Size& size,
    gpu::SharedImageUsageSet usage,
    std::string debug_label,
    Microsoft::WRL::ComPtr<ID3D12Resource> d3d12_buffer,
    Microsoft::WRL::ComPtr<ID3D12Heap> d3d12_heap,
    std::unique_ptr<void, VirtualAllocAddressDeleter> d3d12_heap_memory,
    bool is_thread_safe) {
  auto backing = base::WrapUnique(new D3DImageBacking(
      mailbox, size, usage, std::move(debug_label), std::move(d3d12_buffer),
      std::move(d3d12_heap), std::move(d3d12_heap_memory), is_thread_safe));
  return backing;
}

// static
std::unique_ptr<D3DImageBacking> D3DImageBacking::Create(
    const Mailbox& mailbox,
    const SharedImageInfo& si_info,
    Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture,
    scoped_refptr<DXGISharedHandleState> dxgi_shared_handle_state,
    const GLFormatCaps& gl_format_caps,
    GLenum texture_target,
    size_t array_slice,
    bool use_update_subresource1,
    bool want_dcomp_texture,
    bool is_thread_safe,
    bool share_dxgi_handle_with_other_backings) {
  const bool has_webgpu_usage = si_info.usage.HasAny(
      SHARED_IMAGE_USAGE_WEBGPU_READ | SHARED_IMAGE_USAGE_WEBGPU_WRITE);
  // DXGI shared handle is required for WebGPU/Dawn/D3D12 interop.
  CHECK(!has_webgpu_usage || dxgi_shared_handle_state);
  auto backing = base::WrapUnique(new D3DImageBacking(
      mailbox, si_info, std::move(d3d11_texture),
      std::move(dxgi_shared_handle_state), gl_format_caps, texture_target,
      array_slice, use_update_subresource1, want_dcomp_texture, is_thread_safe,
      share_dxgi_handle_with_other_backings));
  return backing;
}

D3DImageBacking::D3DImageBacking(
    const Mailbox& mailbox,
    const SharedImageInfo& si_info,
    Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture,
    scoped_refptr<DXGISharedHandleState> dxgi_shared_handle_state,
    const GLFormatCaps& gl_format_caps,
    GLenum texture_target,
    size_t array_slice,
    bool use_update_subresource1,
    bool want_dcomp_texture,
    bool is_thread_safe,
    bool share_dxgi_handle_with_other_backings)
    : ClearTrackingSharedImageBacking(
          mailbox,
          si_info,
          si_info.format.EstimatedSizeInBytes(si_info.size),
          is_thread_safe),
      d3d11_texture_(std::move(d3d11_texture)),
      dxgi_shared_handle_state_(std::move(dxgi_shared_handle_state)),
      gl_format_caps_(gl_format_caps),
      texture_target_(texture_target),
      array_slice_(array_slice),
      use_update_subresource1_(use_update_subresource1),
      share_dxgi_handle_with_other_backings_(
          dxgi_shared_handle_state_ && share_dxgi_handle_with_other_backings),
      want_dcomp_texture_(want_dcomp_texture),
      angle_d3d11_device_(gl::QueryD3D11DeviceObjectFromANGLE()) {
  if (d3d11_texture_) {
    d3d11_texture_->GetDevice(&texture_d3d11_device_);
    d3d11_texture_->GetDesc(&d3d11_texture_desc_);
    texture_device_can_use_d3d12_ =
        CanUseD3D12(texture_d3d11_device_.Get(), d3d11_texture_desc_);
  }
}

D3DImageBacking::D3DImageBacking(
    const Mailbox& mailbox,
    const gfx::Size& size,
    gpu::SharedImageUsageSet usage,
    std::string debug_label,
    Microsoft::WRL::ComPtr<ID3D12Resource> d3d12_buffer,
    Microsoft::WRL::ComPtr<ID3D12Heap> d3d12_heap,
    std::unique_ptr<void, VirtualAllocAddressDeleter> d3d12_heap_memory,
    bool is_thread_safe)
    : ClearTrackingSharedImageBacking(
          mailbox,
          SharedImageInfo(viz::SharedImageFormat(),
                          size,
                          gfx::ColorSpace(),
                          GrSurfaceOrigin::kTopLeft_GrSurfaceOrigin,
                          SkAlphaType::kUnknown_SkAlphaType,
                          usage,
                          std::move(debug_label)),
          size.width(),
          is_thread_safe),
      d3d12_heap_memory_(std::move(d3d12_heap_memory)),
      d3d12_heap_(std::move(d3d12_heap)),
      d3d12_buffer_(std::move(d3d12_buffer)),
      texture_target_(0),
      array_slice_(0),
      use_update_subresource1_(false) {}

D3DImageBacking::~D3DImageBacking() {
  if (!have_context()) {
    for (auto& texture_holder : gl_texture_holders_) {
      if (texture_holder) {
        texture_holder->MarkContextLost();
      }
    }
  }
}

ID3D11Texture2D* D3DImageBacking::GetOrCreateStagingTexture() {
  if (!staging_texture_) {
    D3D11_TEXTURE2D_DESC staging_desc = {};
    staging_desc.Width = d3d11_texture_desc_.Width;
    staging_desc.Height = d3d11_texture_desc_.Height;
    staging_desc.Format = d3d11_texture_desc_.Format;
    staging_desc.MipLevels = 1;
    staging_desc.ArraySize = 1;
    staging_desc.SampleDesc.Count = 1;
    staging_desc.Usage = D3D11_USAGE_STAGING;
    staging_desc.CPUAccessFlags =
        D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

    CHECK(texture_d3d11_device_);
    HRESULT hr = texture_d3d11_device_->CreateTexture2D(&staging_desc, nullptr,
                                                        &staging_texture_);
    if (FAILED(hr)) {
      LOG(ERROR) << "Failed to create staging texture. hr=" << std::hex << hr;
      return nullptr;
    }

    constexpr char kStagingTextureLabel[] = "SharedImageD3D_StagingTexture";
    // Add debug label to the long lived texture.
    staging_texture_->SetPrivateData(WKPDID_D3DDebugObjectName,
                                     strlen(kStagingTextureLabel),
                                     kStagingTextureLabel);
  }
  return staging_texture_.Get();
}

SharedImageBackingType D3DImageBacking::GetType() const {
  return SharedImageBackingType::kD3D;
}

void D3DImageBacking::Update(std::unique_ptr<gfx::GpuFence> in_fence) {
  // Do nothing since D3DImageBackings are only ever backed by DXGI GMB handles,
  // which are synonymous with D3D textures, and no explicit update is needed.
}

bool D3DImageBacking::UploadFromMemory(const std::vector<SkPixmap>& pixmaps) {
  AutoLock auto_lock(this);
  DCHECK_EQ(pixmaps.size(), static_cast<size_t>(format().NumberOfPlanes()));

  // Flush any previously deferred Graphite commands before uploading to the
  // D3D11 texture to ensure correct ordering.
  FlushGraphiteCommandsIfNeeded();

  if (use_update_subresource1_ && CanUseUpdateSubresource(pixmaps)) {
    CHECK(texture_d3d11_device_);
    Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1> device_context_1;
    texture_d3d11_device_->GetImmediateContext(&device_context);
    device_context.As(&device_context_1);

    device_context_1->UpdateSubresource1(
        d3d11_texture_.Get(), /*DstSubresource=*/0, /*pDstBox=*/nullptr,
        pixmaps[0].addr(), pixmaps[0].rowBytes(), /*SrcDepthPitch=*/0,
        D3D11_COPY_DISCARD);

    return true;
  }

  ID3D11Texture2D* staging_texture = GetOrCreateStagingTexture();
  if (!staging_texture) {
    return false;
  }

  CHECK(texture_d3d11_device_);
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
  texture_d3d11_device_->GetImmediateContext(&device_context);

  D3D11_MAPPED_SUBRESOURCE mapped_resource = {};
  HRESULT hr = device_context->Map(staging_texture, 0, D3D11_MAP_WRITE, 0,
                                   &mapped_resource);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to map texture for write. hr=" << std::hex << hr;
    return false;
  }

  // The mapped staging texture pData points to the first plane's data so an
  // offset is needed for subsequent planes.
  size_t dest_offset = 0;

  for (int plane = 0; plane < format().NumberOfPlanes(); ++plane) {
    auto& pixmap = pixmaps[plane];
    const uint8_t* source_memory = static_cast<const uint8_t*>(pixmap.addr());
    const size_t source_stride = pixmap.rowBytes();

    uint8_t* dest_memory =
        UNSAFE_TODO(static_cast<uint8_t*>(mapped_resource.pData) + dest_offset);
    const size_t dest_stride = mapped_resource.RowPitch;

    gfx::Size plane_size = format().GetPlaneSize(plane, size());
    CopyImagePlane(source_memory, source_stride, dest_memory, dest_stride,
                   pixmap.info().minRowBytes(), plane_size.height());

    dest_offset += mapped_resource.RowPitch * plane_size.height();
  }

  device_context->Unmap(staging_texture, 0);
  device_context->CopyResource(d3d11_texture_.Get(), staging_texture);

  return true;
}

bool D3DImageBacking::CopyToStagingTexture() {
  TRACE_EVENT0("gpu", "D3DImageBacking::CopyToStagingTexture");
  // Flush any previously deferred Graphite commands to ensure the readback
  // doesn't contain stale data.
  FlushGraphiteCommandsIfNeeded();
  ID3D11Texture2D* staging_texture = GetOrCreateStagingTexture();
  if (!staging_texture) {
    return false;
  }
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
  texture_d3d11_device_->GetImmediateContext(&device_context);
  device_context->CopyResource(staging_texture, d3d11_texture_.Get());
  return true;
}

bool D3DImageBacking::ReadbackFromStagingTexture(
    const std::vector<SkPixmap>& pixmaps) {
  TRACE_EVENT0("gpu", "D3DImageBacking::ReadbackFromStagingTexture");
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
  texture_d3d11_device_->GetImmediateContext(&device_context);

  ID3D11Texture2D* staging_texture = GetOrCreateStagingTexture();

  D3D11_MAPPED_SUBRESOURCE mapped_resource = {};
  HRESULT hr = device_context->Map(staging_texture, 0, D3D11_MAP_READ, 0,
                                   &mapped_resource);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to map texture for read. hr=" << std::hex << hr;
    return false;
  }

  // The mapped staging texture pData points to the first plane's data so an
  // offset is needed for subsequent planes.
  size_t source_offset = 0;

  for (int plane = 0; plane < format().NumberOfPlanes(); ++plane) {
    auto& pixmap = pixmaps[plane];
    uint8_t* dest_memory = static_cast<uint8_t*>(pixmap.writable_addr());
    const size_t dest_stride = pixmap.rowBytes();

    const uint8_t* source_memory = UNSAFE_TODO(
        static_cast<uint8_t*>(mapped_resource.pData) + source_offset);
    const size_t source_stride = mapped_resource.RowPitch;

    gfx::Size plane_size = format().GetPlaneSize(plane, size());
    CopyImagePlane(source_memory, source_stride, dest_memory, dest_stride,
                   pixmap.info().minRowBytes(), plane_size.height());

    source_offset += mapped_resource.RowPitch * plane_size.height();
  }

  device_context->Unmap(staging_texture, 0);
  return true;
}

bool D3DImageBacking::ReadbackToMemory(const std::vector<SkPixmap>& pixmaps) {
  TRACE_EVENT0("gpu", "D3DImageBacking::ReadbackToMemory");
  AutoLock auto_lock(this);
  return CopyToStagingTexture() && ReadbackFromStagingTexture(pixmaps);
}

void D3DImageBacking::ReadbackToMemoryAsync(
    const std::vector<SkPixmap>& pixmaps,
    base::OnceCallback<void(bool)> callback) {
  AutoLock auto_lock(this);
  TRACE_EVENT0("gpu", "D3DImageBacking::ReadbackToMemoryAsync");

  if (pending_copy_event_watcher_) {
    LOG(ERROR) << "Existing ReadbackToMemory operation pending";
    std::move(callback).Run(false);
    return;
  }

  if (!CopyToStagingTexture()) {
    std::move(callback).Run(false);
    return;
  }

  base::WaitableEvent copy_complete_event;
  Microsoft::WRL::ComPtr<IDXGIDevice2> dxgi_device;
  const HRESULT hr = texture_d3d11_device_.As(&dxgi_device);
  CHECK_EQ(hr, S_OK);
  dxgi_device->EnqueueSetEvent(copy_complete_event.handle());

  pending_copy_event_watcher_.emplace();
  CHECK(pending_copy_event_watcher_->StartWatching(
      &copy_complete_event,
      base::IgnoreArgs<base::WaitableEvent*>(base::BindOnce(
          &D3DImageBacking::OnCopyToStagingTextureDone,
          weak_ptr_factory_.GetWeakPtr(), pixmaps, std::move(callback))),
      base::SingleThreadTaskRunner::GetCurrentDefault()));
}

void D3DImageBacking::OnCopyToStagingTextureDone(
    const std::vector<SkPixmap>& pixmaps,
    base::OnceCallback<void(bool)> readback_cb) {
  AutoLock auto_lock(this);
  pending_copy_event_watcher_.reset();
  std::move(readback_cb).Run(ReadbackFromStagingTexture(pixmaps));
}



Microsoft::WRL::ComPtr<ID3D12Resource> D3DImageBacking::EnsureD3D12Resource() {
  if (is_texture_unwrapped_for_d3d12_) {
    return d3d12_resource_;
  }
  auto d3d12_resource = PrepareD3D11on12TextureForD3D12(
      d3d11_texture_.Get(), GetOrCreateCommandQueueFor11On12());
  is_texture_unwrapped_for_d3d12_ = true;
  // We want to avoid churning DCompTextures as much as possible. However,
  // `d3d12_resource` may not equal `d3d12_resource_` in the case where
  // D3D11on12 has decided to allocate a new object for the resource.
  // Example: Developer calls UpdateSubresource to modify a resource that is
  // currently being read or written to by the GPU.
  if (d3d12_resource != d3d12_resource_) {
    dcomp_texture_.Reset();
    d3d12_resource_ = std::move(d3d12_resource);
    // This used to also invalidate a cached Dawn shared texture memory
    // wrapper for the new resource; gone with Dawn.
  }
  return d3d12_resource_;
}


void D3DImageBacking::UpdateExternalFence(
    scoped_refptr<gfx::D3DSharedFence> external_fence) {
  // TODO(crbug.com/40192861): Handle cases that write_fences_ is not empty.
  AutoLock auto_lock(this);
  write_fences_.insert(std::move(external_fence));
}

std::unique_ptr<VideoImageRepresentation> D3DImageBacking::ProduceVideo(
    SharedImageManager* manager,
    MemoryTypeTracker* tracker,
    VideoDevice device) {
  D3D11TextureAndArrayIndex src_texture(d3d11_texture_, array_slice_);
  if (texture_d3d11_device_ != device) {
    // Readback is the only option for a caller cannot create a representation
    // for this shared image.  When the caller cannot use a shared device
    // (GL/Ganesh) create a copy since this is much more efficient than forcing
    // readback.
    return D3D11VideoImageCopyRepresentation::CreateFromD3D(
        manager, this, tracker, device.Get(), src_texture, debug_label(),
        texture_d3d11_device_.Get());
  }

  return std::make_unique<D3DVideoImageRepresentation>(manager, this, tracker,
                                                       device, src_texture);
}








void D3DImageBacking::NotifyGraphiteAboutInitializedStatus() {
  // This used to notify a persistent Dawn SharedTextureMemory access about
  // the texture's initialized status. Dawn is gone, so there is no such
  // access to notify.
}


bool D3DImageBacking::BeginAccessD3D(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device,
    bool write_access,
    bool is_overlay_access) {
  // The backing's D3D12 resource and unwrap state are bound to
  // `texture_d3d11_device_`, so only that device can take the D3D12 path. A
  // different accessing device (only possible with a DXGI shared handle) uses
  // the D3D11 path even if it is itself a D3D11On12 device, since it accesses
  // through its own shared-handle rather than the texture device's resource.
  if (texture_device_can_use_d3d12_ && d3d11_device == texture_d3d11_device_) {
    return BeginAccessD3D12(d3d11_device, write_access, is_overlay_access);
  } else {
    return BeginAccessD3D11(d3d11_device, write_access, is_overlay_access);
  }
}

bool D3DImageBacking::BeginAccessD3D12(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11on12_device,
    bool write_access,
    bool is_overlay_access) {
  AutoLock auto_lock(this);

  FlushGraphiteCommandsIfNeeded();

  if (!ValidateBeginAccess(write_access)) {
    return false;
  }

  auto unwrapped_d3d12_resource = EnsureD3D12Resource();

  // Defer clearing fences until later to handle failure to synchronize.
  auto wait_fences = GetPendingWaitFences(d3d11on12_device, write_access);
  if (!wait_fences) {
    LOG(ERROR) << "Failed to get pending wait fences";
    return false;
  }
  for (auto& wait_fence : *wait_fences) {
    if (!wait_fence->WaitD3D11(d3d11on12_device)) {
      LOG(ERROR) << "Failed to wait for fence";
      return false;
    }
  }

  if (want_dcomp_texture_ && !dcomp_texture_) {
    dcomp_texture_ = CreateDCompTexture(unwrapped_d3d12_resource.Get(),
                                        alpha_type(), color_space());
  }

  if (is_overlay_access && dcomp_texture_) {
    CHECK(!write_access);
    BeginDCompTextureAccess();
  }

  // Clear fences and update state iff D3D11 BeginAccess succeeds.
  BeginAccessCommon(write_access);

  return true;
}

bool D3DImageBacking::BeginAccessD3D11(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device,
    bool write_access,
    bool is_overlay_access) {
  AutoLock auto_lock(this);
  // This used to flush or invalidate a persistent Dawn SharedTextureMemory
  // access here depending on which D3D11 device was accessing the backing.
  // Dawn is gone, so there is no such access to manage.

  // If the texture is currently unwrapped for D3D12, it means we are
  // concurrently accessing the texture on D3D11 and D3D12. This is fine for
  // read-only access, but not allowed for write.
  if (is_texture_unwrapped_for_d3d12_) {
    PrepareD3D11on12TextureForD3D11(d3d11_texture_.Get());
    is_texture_unwrapped_for_d3d12_ = false;
    // Do not release the D3D12 resource here since it may back Dawn's shared
    // texture memory or a DComp texture. This will prevent the need to create a
    // new shared texture or DComp texture the next time we need them.
  }

  if (!ValidateBeginAccess(write_access)) {
    return false;
  }

  // Defer clearing fences until later to handle D3D11 failure to synchronize.
  auto wait_fences = GetPendingWaitFences(d3d11_device, write_access);
  if (!wait_fences) {
    LOG(ERROR) << "Failed to get pending wait fences";
    return false;
  }
  for (auto& wait_fence : *wait_fences) {
    if (!wait_fence->WaitD3D11(d3d11_device)) {
      LOG(ERROR) << "Failed to wait for fence";
      return false;
    }
  }

  // D3D11 access is allowed without shared handle for single device scenarios.
  CHECK(dxgi_shared_handle_state_ || d3d11_device == texture_d3d11_device_);
  if (dxgi_shared_handle_state_) {
    // Trace event for backings with DXGI shared handles (e.g. camera capture
    // textures). Used by the MediaFoundationD3D11VideoCapture trace test.
    TRACE_EVENT0("gpu", "D3DImageBacking::BeginAccessD3D11::DXGISharedHandle");
    if (!dxgi_shared_handle_state_->AcquireKeyedMutex(d3d11_device)) {
      LOG(ERROR) << "Failed to synchronize using keyed mutex";
      return false;
    }
  }

  if (want_dcomp_texture_ && !dcomp_texture_) {
    dcomp_texture_ =
        CreateDCompTexture(d3d11_texture_.Get(), alpha_type(), color_space());
  }

  if (is_overlay_access && dcomp_texture_) {
    CHECK(!write_access);
    BeginDCompTextureAccess();
  }

  // Clear fences and update state iff D3D11 BeginAccess succeeds.
  BeginAccessCommon(write_access);

  return true;
}

void D3DImageBacking::EndAccessD3D(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device,
    bool is_overlay_access) {
  const bool is_texture_device = d3d11_device == texture_d3d11_device_;
  // If shared handle is not present, we can only access on the same device.
  AutoLock auto_lock(this);
  CHECK(dxgi_shared_handle_state_ || is_texture_device);

  // Do not create a fence for the texture's original device if we're only using
  // the texture on one device or using a keyed mutex. The fence is lazily
  // created on the first access from another device in GetPendingWaitFences().
  D3DSharedFenceSet signaled_fence;
  if (use_cross_device_fence_synchronization()) {
    auto& d3d11_signal_fence = d3d11_signaled_fence_map_[d3d11_device];
    // If the accessing device was not the texture's original device, create
    // the fence so that future access of the backing waits for all pending work
    // to be completed on `EndAccess`. This is necessary in the case where Dawn
    // may need to access the backing after WebGL has accessed it for write, and
    // therefore must wait for WebGL work to be completed.
    if (!d3d11_signal_fence && (d3d11_device != texture_d3d11_device_)) {
      d3d11_signal_fence = gfx::D3DSharedFence::CreateForD3D11(d3d11_device);
    }
    if (d3d11_signal_fence) {
      if (d3d11_signal_fence->IncrementAndSignalD3D11()) {
        signaled_fence.insert(d3d11_signal_fence);
      } else {
        LOG(ERROR) << "Failed to signal D3D11 device fence on EndAccess";
      }
    }
  }

  if (dxgi_shared_handle_state_ &&
      dxgi_shared_handle_state_->has_keyed_mutex()) {
    dxgi_shared_handle_state_->ReleaseKeyedMutex(d3d11_device);
  }

  if (is_overlay_access && dcomp_texture_) {
    EndDCompTextureAccess();
  }

  if (in_write_access_) {
    NotifyGraphiteAboutInitializedStatus();
  }

  EndAccessCommon(signaled_fence);
}

void D3DImageBacking::BeginDCompTextureAccess() {
  CHECK(dcomp_texture_);
  num_dcomp_texture_readers_++;

  if (num_dcomp_texture_readers_ > 1) {
    // If the DComp texture is already in a visual tree, the available fence is
    // invalid and should not be stored.
    CHECK(!dcomp_texture_available_fence_);
  }

  if (dcomp_texture_available_fence_) {
    // When we are putting the DComp texture back into a visual tree, we expect
    // no other holders of the available fence (which can only be outstanding
    // writers).
    CHECK(dcomp_texture_available_fence_->HasOneRef());
    // A new overlay access invalidates the available fence because it implies
    // that `dcomp_texture_` is going back into the visual tree.
    dcomp_texture_available_fence_.reset();
  }
}

void D3DImageBacking::EndDCompTextureAccess() {
  CHECK(dcomp_texture_);
  CHECK_GT(num_dcomp_texture_readers_, 0);
  num_dcomp_texture_readers_--;

  if (num_dcomp_texture_readers_ > 0) {
    // This DComp texture is in another tree.
    return;
  }

  uint64_t fence_value = 0;
  if (texture_device_can_use_d3d12_) {
    Microsoft::WRL::ComPtr<ID3D12Fence> d3d12_fence;
    if (!ShouldWaitForDCompTextureFence(d3d12_fence, fence_value,
                                        dcomp_texture_.Get())) {
      return;
    }
    CHECK(!dcomp_texture_available_fence_);
    dcomp_texture_available_fence_ = gfx::D3DSharedFence::CreateFromD3D12Fence(
        std::move(d3d12_fence), fence_value);
  } else {
    Microsoft::WRL::ComPtr<ID3D11Fence> d3d11_fence;
    if (!ShouldWaitForDCompTextureFence(d3d11_fence, fence_value,
                                        dcomp_texture_.Get())) {
      return;
    }
    // Note we're passing a null device since the DWM internal device will
    // signal this fence.
    CHECK(!dcomp_texture_available_fence_);
    dcomp_texture_available_fence_ = gfx::D3DSharedFence::CreateFromD3D11Fence(
        /*d3d11_signal_device=*/nullptr, std::move(d3d11_fence), fence_value);
  }
}

std::optional<scoped_refptr<gfx::D3DSharedFence>>
D3DImageBacking::BeginAccessWebNN() {
  AutoLock auto_lock(this);

  // WebNNTensors only support exclusive read-write access.
  if (!ValidateBeginAccess(true)) {
    return std::nullopt;
  }

  scoped_refptr<gfx::D3DSharedFence> write_fence;
  if (!write_fences_.empty()) {
    // WebNNTensors expect to wait on 1 fence that originates from Dawn.
    // If there was WebGPU work, this will be Dawn's submission fence.
    // Otherwise, it will return WebNN's submission fence that was previously
    // passed to BeginAccessDawn.
    CHECK_EQ(write_fences_.size(), 1u);
    write_fence = *write_fences_.begin();
  }

  BeginAccessCommon(true);
  return write_fence;
}

void D3DImageBacking::EndAccessWebNN(
    scoped_refptr<gfx::D3DSharedFence> signaled_fence) {
  AutoLock auto_lock(this);
  if (!signaled_fence) {
    EndAccessCommon(/*signaled_fences=*/{});
    return;
  }
  EndAccessCommon({signaled_fence});
}




bool D3DImageBacking::ValidateBeginAccess(bool write_access) const {
  if (usage().Has(SHARED_IMAGE_USAGE_CONCURRENT_READ_WRITE)) {
    // If this backing is being used for concurrent read/write, the only
    // access pattern that's not allowed is concurrent writes.
    if (in_write_access_ && write_access) {
      LOG(ERROR) << "Already being accessed for write";
      return false;
    }
    return true;
  }

  if (in_write_access_) {
    LOG(ERROR) << "Already being accessed for write";
    return false;
  }
  if (write_access && num_readers_ > 0) {
    LOG(ERROR) << "Already being accessed for read";
    return false;
  }
  return true;
}


std::optional<D3DImageBacking::D3DSharedFenceSet>
D3DImageBacking::GetPendingWaitFences(
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device,
    bool write_access) {
  D3DSharedFenceSet wait_fences;

  // The previous write must always be waited on. A write access must also
  // wait on all previous reads.
  wait_fences.insert(write_fences_.begin(), write_fences_.end());
  if (write_access) {
    wait_fences.insert(read_fences_.begin(), read_fences_.end());
  }

  if (use_cross_device_fence_synchronization()) {
    // A different device may have signaled its own per-device fence on
    // EndAccess (see EndAccessD3D) that `d3d11_device` hasn't necessarily
    // observed yet.
    for (const auto& [device, fence] : d3d11_signaled_fence_map_) {
      if (fence && device != d3d11_device) {
        wait_fences.insert(fence);
      }
    }
  }

  return wait_fences;
}

void D3DImageBacking::BeginAccessCommon(bool write_access) {
  if (write_access) {
    // For read-write access, we wait for all previous reads and reset fences
    // since all subsequent access will wait on |write_fence_| generated when
    // this access ends.
    write_fences_.clear();
    read_fences_.clear();
    in_write_access_ = true;
  } else {
    num_readers_++;
  }
}

void D3DImageBacking::EndAccessCommon(
    const D3DSharedFenceSet& signaled_fences) {
  DCHECK(std::ranges::all_of(
      signaled_fences,
      [](const scoped_refptr<gfx::D3DSharedFence>& fence) { return !!fence; }));
  if (in_write_access_) {
    DCHECK(write_fences_.empty());
    DCHECK(read_fences_.empty());
    in_write_access_ = false;
    write_fences_ = signaled_fences;

    // If this backing is holding both buffers of a swapchain (i.e., being used
    // for concurrent read/write), ensure that the contents of the write are
    // presented as soon as possible by calling Present() on the swapchain. Note
    // that it is necessary to do this here rather than in EndAccessD3D() to
    // ensure that this executes if Graphite is being used.
    if (swap_chain_front_buffer_texture_) {
      PresentSwapChain();

      // Copy from front buffer to back buffer to ensure that contents are
      // preserved for subsequent reads from the back buffer.
      Microsoft::WRL::ComPtr<ID3D11DeviceContext> device_context;
      texture_d3d11_device_->GetImmediateContext(&device_context);
      device_context->CopyResource(d3d11_texture_.Get(),
                                   swap_chain_front_buffer_texture_.Get());
    }
  } else {
    num_readers_--;
    for (const auto& signaled_fence : signaled_fences) {
      read_fences_.insert(signaled_fence);
    }
  }
}

void* D3DImageBacking::GetEGLImage() const {
  AutoLock auto_lock(this);
  DCHECK(format().is_single_plane());
  return gl_texture_holders_[0] ? gl_texture_holders_[0]->egl_image() : nullptr;
}

bool D3DImageBacking::PresentSwapChain() {
  AutoLock auto_lock(this);
  if (!swap_chain_) {
    LOG(ERROR) << "Backing is not holding swap chain";
    return false;
  }

  // Flush any deferred Graphite submits before presentation.
  FlushGraphiteCommandsIfNeeded();

  TRACE_EVENT1("gpu", "D3DImageBacking::PresentSwapChain", "has_alpha",
               !SkAlphaTypeIsOpaque(alpha_type()));
  constexpr UINT kFlags = DXGI_PRESENT_ALLOW_TEARING;
  constexpr DXGI_PRESENT_PARAMETERS kParams = {};
  HRESULT hr = swap_chain_->Present1(/*interval=*/0, kFlags, &kParams);
  if (FAILED(hr)) {
    LOG(ERROR) << "Present1 failed with error " << std::hex << hr;
    return false;
  }

  DCHECK(format().is_single_plane());

  // we're rebinding to ensure that underlying D3D11 resource views are
  // recreated in ANGLE.
  // TODO(crbug.com/40074896): Determine whether we need to do something similar
  // for Dawn when using Graphite.
  if (gl_texture_holders_[0]) {
    gl_texture_holders_[0]->set_needs_rebind(true);
  }

  // Flush device context otherwise present could be deferred.
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_device_context;
  texture_d3d11_device_->GetImmediateContext(&d3d11_device_context);
  d3d11_device_context->Flush();

  return true;
}

std::unique_ptr<GLTexturePassthroughImageRepresentation>
D3DImageBacking::ProduceGLTexturePassthrough(SharedImageManager* manager,
                                             MemoryTypeTracker* tracker) {
  TRACE_EVENT0("gpu", "D3DImageBacking::ProduceGLTexturePassthrough");

  const auto number_of_planes = static_cast<size_t>(format().NumberOfPlanes());
  std::vector<scoped_refptr<GLTextureHolder>> gl_texture_holders(
      number_of_planes);
  {
    AutoLock auto_lock(this);
    DCHECK_GE(gl_texture_holders_.size(), number_of_planes);

    // If DXGI shared handle is present, the |d3d11_texture_| might belong to a
    // different device with Graphite so retrieve the ANGLE specific D3D11
    // texture from the |dxgi_shared_handle_state_|.
    const bool is_angle_texture = texture_d3d11_device_ == angle_d3d11_device_;
    CHECK(is_angle_texture || dxgi_shared_handle_state_);
    auto d3d11_texture =
        is_angle_texture ? d3d11_texture_
                         : dxgi_shared_handle_state_->GetOrCreateD3D11Texture(
                               angle_d3d11_device_);
    if (!d3d11_texture) {
      LOG(ERROR) << "Failed to open DXGI shared handle";
      return nullptr;
    }

    for (int plane = 0; plane < format().NumberOfPlanes(); ++plane) {
      auto& holder = gl_texture_holders[plane];
      if (gl_texture_holders_[plane]) {
        holder = gl_texture_holders_[plane].get();
        continue;
      }

      // The GL internal format can differ from the underlying swap chain or
      // texture format e.g. RGBA or RGB instead of BGRA or RED/RG for NV12
      // texture planes. See EGL_ANGLE_d3d_texture_client_buffer spec for format
      // restrictions.
      GLFormatDesc gl_format_desc;
      if (format().is_multi_plane()) {
        gl_format_desc = gl_format_caps_.ToGLFormatDesc(format(), plane);
      } else {
        // For legacy multiplanar formats, `format` is already plane format (eg.
        // RED, RG), so we pass plane_index=0.
        gl_format_desc =
            gl_format_caps_.ToGLFormatDesc(format(), /*plane_index=*/0);
      }

      // Creating the GL texture doesn't require exclusive access to the
      // underlying D3D11 texture.
      holder = CreateGLTexture(gl_format_desc, d3d11_texture, texture_target_,
                               array_slice_, plane);
      if (!holder) {
        LOG(ERROR) << "Failed to create GL texture for plane: " << plane;
        return nullptr;
      }
      // Cache the gl textures using weak pointers.
      gl_texture_holders_[plane] = holder->GetWeakPtr();
    }
  }

  return std::make_unique<GLTexturePassthroughD3DImageRepresentation>(
      manager, this, tracker, angle_d3d11_device_,
      std::move(gl_texture_holders));
}

std::unique_ptr<SkiaGaneshImageRepresentation>
D3DImageBacking::ProduceSkiaGanesh(
    SharedImageManager* manager,
    MemoryTypeTracker* tracker,
    scoped_refptr<SharedContextState> context_state) {
  auto gl_representation = ProduceGLTexturePassthrough(manager, tracker);
  if (!gl_representation) {
    return nullptr;
  }
  return SkiaGLImageRepresentation::Create(std::move(gl_representation),
                                           std::move(context_state), manager,
                                           this, tracker);
}

// ProduceSkiaGraphite used to wrap the D3D texture as a Dawn texture. Dawn
// is gone, so this backing no longer overrides it; the base class
// implementation (returns nullptr) is used instead.

std::unique_ptr<OverlayImageRepresentation> D3DImageBacking::ProduceOverlay(
    SharedImageManager* manager,
    MemoryTypeTracker* tracker) {
  TRACE_EVENT0("gpu", "D3DImageBacking::ProduceOverlay");
  return std::make_unique<OverlayD3DImageRepresentation>(manager, this, tracker,
                                                         texture_d3d11_device_);
}

std::optional<gl::DCLayerOverlayImage>
D3DImageBacking::GetDCLayerOverlayImage() {
  if (dcomp_texture_) {
    return std::make_optional<gl::DCLayerOverlayImage>(
        size(), dcomp_texture_, /*dcomp_surface_serial=*/0);
  }
  if (swap_chain_) {
    return std::make_optional<gl::DCLayerOverlayImage>(size(), swap_chain_);
  }
  return std::make_optional<gl::DCLayerOverlayImage>(size(), d3d11_texture_,
                                                     array_slice_);
}

Microsoft::WRL::ComPtr<ID3D12Resource> D3DImageBacking::GetD3D12Buffer() const {
  return d3d12_buffer_;
}

base::win::ScopedHandle D3DImageBacking::GetD3D12HeapHandle() const {
  if (!d3d12_heap_) {
    return base::win::ScopedHandle();
  }
  Microsoft::WRL::ComPtr<ID3D12Device> d3d12_device;
  CHECK_EQ(d3d12_heap_->GetDevice(IID_PPV_ARGS(&d3d12_device)), S_OK);

  HANDLE shared_handle = nullptr;
  // TODO(crbug.com/419598085): Cache the shared handle.
  CHECK_EQ(
      d3d12_device->CreateSharedHandle(d3d12_heap_.Get(), nullptr, GENERIC_ALL,
                                       nullptr, &shared_handle),
      S_OK);
  return base::win::ScopedHandle(shared_handle);
}

bool D3DImageBacking::HasStagingTextureForTesting() const {
  AutoLock auto_lock(this);
  return !!staging_texture_;
}

void D3DImageBacking::FlushGraphiteCommandsIfNeeded() {
  // This used to flush a persistent Dawn SharedTextureMemory access's
  // pending Graphite submits. Dawn is gone, so there is no such access.
}

void D3DImageBacking::InvalidatePersistentGraphiteDawnAccess() {
  // This used to force-EndAccess a persistent Dawn SharedTextureMemory
  // access. Dawn is gone, so there is no such access. (No longer called;
  // kept declared in case that changes.)
}

bool D3DImageBacking::SupportsDeferredGraphiteSubmit() const {
  // This used to be true when a persistent Dawn Graphite access was live.
  // Dawn is gone, so deferred submits are never supported.
  return false;
}

ID3D12CommandQueue* D3DImageBacking::GetOrCreateCommandQueueFor11On12() {
  if (d3d12_texture_unwrap_command_queue_) {
    return d3d12_texture_unwrap_command_queue_.Get();
  }

  Microsoft::WRL::ComPtr<ID3D11On12Device2> d3d11on12_device;
  HRESULT hr = texture_d3d11_device_.As(&d3d11on12_device);
  CHECK_EQ(hr, S_OK);

  D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
  command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_NONE;
  command_queue_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
  command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
  command_queue_desc.NodeMask = 0;

  Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue;
  Microsoft::WRL::ComPtr<ID3D12Device> d3d12_device;
  d3d11on12_device->GetD3D12Device(IID_PPV_ARGS(&d3d12_device));
  // `CreateCommandQueue` with D3D12_COMMAND_QUEUE_FLAG_NONE should always
  // succeed, even if the device has been removed.
  hr = d3d12_device->CreateCommandQueue(&command_queue_desc,
                                        IID_PPV_ARGS(&command_queue));
  CHECK_EQ(hr, S_OK);
  d3d12_texture_unwrap_command_queue_ = std::move(command_queue);
  return d3d12_texture_unwrap_command_queue_.Get();
}

}  // namespace gpu
