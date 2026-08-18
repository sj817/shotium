// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image/d3d_image_representation.h"

#include "base/strings/strcat.h"
#include "gpu/command_buffer/common/constants.h"
#include "gpu/command_buffer/common/shared_image_usage.h"
#include "gpu/command_buffer/service/shared_image/d3d11_image_same_adapter_copy_strategy.h"
#include "gpu/command_buffer/service/shared_image/d3d_image_backing.h"
#include "gpu/ipc/common/dxgi_helpers.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"
#include "third_party/angle/include/EGL/eglext_angle.h"
#include "ui/gl/gl_angle_util_win.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/scoped_restore_texture.h"

namespace {

D3D11_TEXTURE2D_DESC InitVideoCopyTextureDesc(UINT width,
                                              UINT height,
                                              DXGI_FORMAT format) {
  D3D11_TEXTURE2D_DESC desc = {0};
  desc.Width = width;
  desc.Height = height;
  desc.Format = format;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;
  desc.Usage = D3D11_USAGE_DEFAULT;
  desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = 0;
  desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED_NTHANDLE |
                   D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;

  return desc;
}

}  // namespace

namespace gpu {

GLTexturePassthroughD3DImageRepresentation::
    GLTexturePassthroughD3DImageRepresentation(
        SharedImageManager* manager,
        SharedImageBacking* backing,
        MemoryTypeTracker* tracker,
        Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device,
        std::vector<scoped_refptr<D3DImageBacking::GLTextureHolder>>
            gl_texture_holders)
    : GLTexturePassthroughImageRepresentation(manager, backing, tracker),
      d3d11_device_(std::move(d3d11_device)),
      gl_texture_holders_(std::move(gl_texture_holders)) {}

GLTexturePassthroughD3DImageRepresentation::
    ~GLTexturePassthroughD3DImageRepresentation() = default;

bool GLTexturePassthroughD3DImageRepresentation::
    NeedsSuspendAccessForDXGIKeyedMutex() const {
  return static_cast<D3DImageBacking*>(backing())->has_keyed_mutex();
}

const scoped_refptr<gles2::TexturePassthrough>&
GLTexturePassthroughD3DImageRepresentation::GetTexturePassthrough(
    size_t plane_index) {
  return gl_texture_holders_[plane_index]->texture_passthrough();
}

void* GLTexturePassthroughD3DImageRepresentation::GetEGLImage() {
  DCHECK(format().is_single_plane());
  return gl_texture_holders_[0]->egl_image();
}

bool GLTexturePassthroughD3DImageRepresentation::BeginAccess(GLenum mode) {
  D3DImageBacking* d3d_image_backing = static_cast<D3DImageBacking*>(backing());

  const bool write_access =
      mode == GL_SHARED_IMAGE_ACCESS_MODE_READWRITE_CHROMIUM;
  if (!d3d_image_backing->BeginAccessD3D(d3d11_device_, write_access)) {
    return false;
  }

  for (auto& gl_texture_holder : gl_texture_holders_) {
    // Bind GLImage to texture if it is necessary.
    gl_texture_holder->BindEGLImageToTexture();
  }

  return true;
}

void GLTexturePassthroughD3DImageRepresentation::EndAccess() {
  D3DImageBacking* d3d_image_backing = static_cast<D3DImageBacking*>(backing());
  d3d_image_backing->EndAccessD3D(d3d11_device_);
}


// DawnD3DImageRepresentation and DawnD3DBufferRepresentation are gone with
// Dawn.


// WebNND3DTensorRepresentation is gone with //services/webnn.

OverlayD3DImageRepresentation::OverlayD3DImageRepresentation(
    SharedImageManager* manager,
    SharedImageBacking* backing,
    MemoryTypeTracker* tracker,
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device)
    : OverlayImageRepresentation(manager, backing, tracker),
      d3d11_device_(std::move(d3d11_device)) {}

OverlayD3DImageRepresentation::~OverlayD3DImageRepresentation() = default;

bool OverlayD3DImageRepresentation::BeginReadAccess(
    gfx::GpuFenceHandle& acquire_fence) {
  return static_cast<D3DImageBacking*>(backing())->BeginAccessD3D(
      d3d11_device_, /*write_access=*/false, /*is_overlay=*/true);
}

void OverlayD3DImageRepresentation::EndReadAccess(
    gfx::GpuFenceHandle release_fence) {
  DCHECK(release_fence.is_null());
  static_cast<D3DImageBacking*>(backing())->EndAccessD3D(d3d11_device_,
                                                         /*is_overlay=*/true);
}

std::optional<gl::DCLayerOverlayImage>
OverlayD3DImageRepresentation::GetDCLayerOverlayImage() {
  return static_cast<D3DImageBacking*>(backing())->GetDCLayerOverlayImage();
}

D3DVideoImageRepresentation::D3DVideoImageRepresentation(
    SharedImageManager* manager,
    SharedImageBacking* backing,
    MemoryTypeTracker* tracker,
    Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device,
    D3D11TextureAndArrayIndex d3d11_texture)
    : VideoImageRepresentation(manager, backing, tracker),
      d3d11_device_(std::move(d3d11_device)),
      d3d11_texture_(std::move(d3d11_texture)) {}

D3DVideoImageRepresentation::~D3DVideoImageRepresentation() = default;

bool D3DVideoImageRepresentation::BeginWriteAccess() {
  D3DImageBacking* d3d_image_backing = static_cast<D3DImageBacking*>(backing());
  if (!d3d_image_backing->BeginAccessD3D(d3d11_device_,
                                         /*write_access=*/true)) {
    return false;
  }
  return true;
}

void D3DVideoImageRepresentation::EndWriteAccess() {
  D3DImageBacking* d3d_image_backing = static_cast<D3DImageBacking*>(backing());
  d3d_image_backing->EndAccessD3D(d3d11_device_);
}

bool D3DVideoImageRepresentation::BeginReadAccess() {
  D3DImageBacking* d3d_image_backing = static_cast<D3DImageBacking*>(backing());
  if (!d3d_image_backing->BeginAccessD3D(d3d11_device_,
                                         /*write_access=*/false)) {
    return false;
  }
  return true;
}

void D3DVideoImageRepresentation::EndReadAccess() {
  D3DImageBacking* d3d_image_backing = static_cast<D3DImageBacking*>(backing());
  d3d_image_backing->EndAccessD3D(d3d11_device_);
}

D3D11TextureAndArrayIndex D3DVideoImageRepresentation::GetD3D11Texture() const {
  return d3d11_texture_;
}

std::unique_ptr<D3D11VideoImageCopyRepresentation>
D3D11VideoImageCopyRepresentation::CreateFromGL(GLuint gl_texture_id,
                                                std::string_view debug_label,
                                                ID3D11Device* d3d_device,
                                                SharedImageManager* manager,
                                                SharedImageBacking* backing,
                                                MemoryTypeTracker* tracker) {
  gl::GLApi* const api = gl::g_current_gl_context;
  D3D11_TEXTURE2D_DESC desc = InitVideoCopyTextureDesc(
      backing->size().width(), backing->size().height(),
      DXGI_FORMAT_R8G8B8A8_UNORM);

  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d_dest_texture;
  HRESULT hr = d3d_device->CreateTexture2D(&desc, nullptr, &d3d_dest_texture);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to create destination texture for video: "
               << logging::SystemErrorCodeToString(hr);
    return nullptr;
  }
  std::string updated_debug_label = base::StrCat(
      {"D3D11VideoImageCopyRepresentation_", std::string(debug_label)});
  d3d_dest_texture->SetPrivateData(WKPDID_D3DDebugObjectName,
                                   updated_debug_label.length(),
                                   updated_debug_label.c_str());

  Microsoft::WRL::ComPtr<IDXGIResource1> dxgi_resource;
  hr = d3d_dest_texture.As(&dxgi_resource);
  CHECK_EQ(hr, S_OK);
  HANDLE dest_texture_handle;
  hr = dxgi_resource->CreateSharedHandle(
      nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr,
      &dest_texture_handle);
  CHECK_EQ(hr, S_OK);
  base::win::ScopedHandle scoped_shared_handle(dest_texture_handle);

  Microsoft::WRL::ComPtr<ID3D11Device> angle_device =
      gl::QueryD3D11DeviceObjectFromANGLE();
  Microsoft::WRL::ComPtr<ID3D11Device1> angle_device1;
  hr = angle_device->QueryInterface(IID_PPV_ARGS(&angle_device1));
  CHECK_EQ(hr, S_OK);
  Microsoft::WRL::ComPtr<ID3D11Texture2D> remote_texture;
  hr = angle_device1->OpenSharedResource1(dest_texture_handle,
                                          IID_PPV_ARGS(&remote_texture));
  CHECK_EQ(hr, S_OK);

  GLuint gl_texture_dest;
  api->glGenTexturesFn(1, &gl_texture_dest);
  gl::ScopedRestoreTexture scoped_restore(gl::g_current_gl_context,
                                          GL_TEXTURE_2D, gl_texture_dest);
  api->glTexParameteriFn(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  api->glTexParameteriFn(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  const EGLint egl_attrib_list[] = {EGL_TEXTURE_INTERNAL_FORMAT_ANGLE, GL_RGBA,
                                    EGL_NONE};
  auto egl_image = gl::MakeScopedEGLImage(
      EGL_NO_CONTEXT, EGL_D3D11_TEXTURE_ANGLE,
      static_cast<EGLClientBuffer>(remote_texture.Get()), egl_attrib_list);
  if (!egl_image.get()) {
    LOG(ERROR) << "Failed to create an EGL image";
    api->glDeleteTexturesFn(1, &gl_texture_dest);
    return nullptr;
  }

  api->glEGLImageTargetTexture2DOESFn(GL_TEXTURE_2D, egl_image.get());
  if (eglGetError() != static_cast<EGLint>(EGL_SUCCESS)) {
    LOG(ERROR) << "Failed to bind EGL image to texture for video";
    api->glDeleteTexturesFn(1, &gl_texture_dest);
    return nullptr;
  }

  Microsoft::WRL::ComPtr<IDXGIKeyedMutex> keyed_mutex;
  hr = remote_texture->QueryInterface(IID_PPV_ARGS(&keyed_mutex));
  CHECK_EQ(hr, S_OK);
  // Using the keyed mutex here is not required for synchronization with another
  // thread; the texture is only written here and only consumed by the decoder
  // using this representation.  However, a side effect of
  // AcquireSync/ReleaseSync is that  modifications to the texture made here are
  // "visible" to other D3D devices.
  hr = keyed_mutex->AcquireSync(0, INFINITE);
  CHECK_EQ(hr, S_OK);
  {
    DXGIScopedReleaseKeyedMutex scoped_keyed_mutex(keyed_mutex, 0);

    // Using glCopySubTextureCHROMIUM may look odd here since the entire texture
    // is being copied.  However, glCopyTextureCHROMIUM for some reason releases
    // the backing texture before copying, which is not helpful since the goal
    // here is to copy to the shared texture and not some random texture that
    // ANGLE creates.
    api->glCopySubTextureCHROMIUMFn(
        gl_texture_id, 0, GL_TEXTURE_2D, gl_texture_dest, 0, 0, 0, 0, 0,
        backing->size().width(), backing->size().height(), GL_FALSE, GL_FALSE,
        GL_FALSE);
    CHECK_EQ(glGetError(), static_cast<unsigned int>(GL_NO_ERROR));
  }

  api->glDeleteTexturesFn(1, &gl_texture_dest);

  return std::make_unique<D3D11VideoImageCopyRepresentation>(
      manager, backing, tracker, d3d_dest_texture.Get());
}

std::unique_ptr<D3D11VideoImageCopyRepresentation>
D3D11VideoImageCopyRepresentation::CreateFromD3D(
    SharedImageManager* manager,
    SharedImageBacking* backing,
    MemoryTypeTracker* tracker,
    ID3D11Device* d3d_device,
    D3D11TextureAndArrayIndex src_texture,
    std::string_view debug_label,
    ID3D11Device* texture_device) {
  auto* d3d_backing = static_cast<D3DImageBacking*>(backing);
  if (!d3d_backing->BeginAccessD3D(texture_device, /*write_access=*/false,
                                   /*is_overlay_access=*/false)) {
    return nullptr;
  }
  absl::Cleanup end_access = [&] {
    d3d_backing->EndAccessD3D(texture_device, /*is_overlay_access=*/false);
  };

  D3D11_TEXTURE2D_DESC source_desc;
  src_texture.texture->GetDesc(&source_desc);

  D3D11_TEXTURE2D_DESC dest_desc = InitVideoCopyTextureDesc(
      source_desc.Width, source_desc.Height, source_desc.Format);

  Microsoft::WRL::ComPtr<ID3D11Texture2D> dest_texture;
  HRESULT hr = d3d_device->CreateTexture2D(&dest_desc, nullptr, &dest_texture);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to create destination texture for video: "
               << logging::SystemErrorCodeToString(hr);
    return nullptr;
  }

  std::string updated_debug_label = base::StrCat(
      {"D3D11VideoImageCopyRepresentation_", std::string(debug_label)});
  dest_texture->SetPrivateData(WKPDID_D3DDebugObjectName,
                               updated_debug_label.length(),
                               updated_debug_label.c_str());

  if (!D3D11ImageSameAdapterCopyStrategy::CopyD3D11TextureOnSameAdapter(
          src_texture, dest_texture.Get())) {
    LOG(ERROR) << "Failed to copy texture for video";
    return nullptr;
  }

  return std::make_unique<D3D11VideoImageCopyRepresentation>(
      manager, backing, tracker, std::move(dest_texture));
}

D3D11VideoImageCopyRepresentation::D3D11VideoImageCopyRepresentation(
    SharedImageManager* manager,
    SharedImageBacking* backing,
    MemoryTypeTracker* tracker,
    Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture)
    : VideoImageRepresentation(manager, backing, tracker),
      d3d11_texture_(std::move(d3d11_texture)) {}

D3D11VideoImageCopyRepresentation::~D3D11VideoImageCopyRepresentation() =
    default;

bool D3D11VideoImageCopyRepresentation::BeginWriteAccess() {
  // This is a copy of an image, write not supported
  NOTREACHED();
}

void D3D11VideoImageCopyRepresentation::EndWriteAccess() {}

bool D3D11VideoImageCopyRepresentation::BeginReadAccess() {
  // This representation and underlying texture was created exclusively
  // for a particular caller.  No synchronization is required because
  // only the caller has this representation.
  return true;
}

void D3D11VideoImageCopyRepresentation::EndReadAccess() {}

D3D11TextureAndArrayIndex D3D11VideoImageCopyRepresentation::GetD3D11Texture()
    const {
  return D3D11TextureAndArrayIndex(d3d11_texture_, /*array_index=*/0);
}





}  // namespace gpu
