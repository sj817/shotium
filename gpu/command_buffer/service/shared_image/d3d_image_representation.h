// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GPU_COMMAND_BUFFER_SERVICE_SHARED_IMAGE_D3D_IMAGE_REPRESENTATION_H_
#define GPU_COMMAND_BUFFER_SERVICE_SHARED_IMAGE_D3D_IMAGE_REPRESENTATION_H_

#include "base/memory/raw_ptr.h"
#include "gpu/command_buffer/service/memory_tracking.h"
#include "gpu/command_buffer/service/shared_image/d3d_image_backing.h"
#include "gpu/command_buffer/service/shared_image/shared_image_representation.h"
#include "gpu/command_buffer/service/texture_manager.h"
#include "ui/gl/buildflags.h"

namespace gpu {

// Representation of a D3DImageBacking as a GL TexturePassthrough.
class GLTexturePassthroughD3DImageRepresentation
    : public GLTexturePassthroughImageRepresentation {
 public:
  GLTexturePassthroughD3DImageRepresentation(
      SharedImageManager* manager,
      SharedImageBacking* backing,
      MemoryTypeTracker* tracker,
      Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device,
      std::vector<scoped_refptr<D3DImageBacking::GLTextureHolder>>
          texture_holders);
  ~GLTexturePassthroughD3DImageRepresentation() override;

  bool NeedsSuspendAccessForDXGIKeyedMutex() const override;

  const scoped_refptr<gles2::TexturePassthrough>& GetTexturePassthrough(
      size_t plane_index) override;

  void* GetEGLImage();

 private:
  bool BeginAccess(GLenum mode) override;
  void EndAccess() override;

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;

  // Holds a gles2::TexturePassthrough and corresponding egl image.
  std::vector<scoped_refptr<D3DImageBacking::GLTextureHolder>>
      gl_texture_holders_;
};




// Representation of a D3DImageBacking as an overlay.
class OverlayD3DImageRepresentation : public OverlayImageRepresentation {
 public:
  OverlayD3DImageRepresentation(
      SharedImageManager* manager,
      SharedImageBacking* backing,
      MemoryTypeTracker* tracker,
      Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device);
  ~OverlayD3DImageRepresentation() override;

 private:
  bool BeginReadAccess(gfx::GpuFenceHandle& acquire_fence) override;
  void EndReadAccess(gfx::GpuFenceHandle release_fence) override;

  std::optional<gl::DCLayerOverlayImage> GetDCLayerOverlayImage() override;

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;
};

class D3DVideoImageRepresentation : public VideoImageRepresentation {
 public:
  D3DVideoImageRepresentation(SharedImageManager* manager,
                              SharedImageBacking* backing,
                              MemoryTypeTracker* tracker,
                              Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device,
                              D3D11TextureAndArrayIndex d3d11_texture);
  ~D3DVideoImageRepresentation() override;

 private:
  bool BeginWriteAccess() override;
  void EndWriteAccess() override;
  bool BeginReadAccess() override;
  void EndReadAccess() override;
  D3D11TextureAndArrayIndex GetD3D11Texture() const override;

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;
  D3D11TextureAndArrayIndex d3d11_texture_;
};

class D3D11VideoImageCopyRepresentation : public VideoImageRepresentation {
 public:
  // Creates a copy of a (D3D-backed) GL texture for use in video encode.
  // This avoids expensive readback.
  static std::unique_ptr<D3D11VideoImageCopyRepresentation> CreateFromGL(
      GLuint gl_texture_id,
      std::string_view debug_label,
      ID3D11Device* d3d_device,
      SharedImageManager* manager,
      SharedImageBacking* backing,
      MemoryTypeTracker* tracker);
  static std::unique_ptr<D3D11VideoImageCopyRepresentation> CreateFromD3D(
      SharedImageManager* manager,
      SharedImageBacking* backing,
      MemoryTypeTracker* tracker,
      ID3D11Device* d3d_device,
      D3D11TextureAndArrayIndex src_texture,
      std::string_view debug_label,
      ID3D11Device* texture_device);

  D3D11VideoImageCopyRepresentation(
      SharedImageManager* manager,
      SharedImageBacking* backing,
      MemoryTypeTracker* tracker,
      Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture);
  ~D3D11VideoImageCopyRepresentation() override;

 private:
  bool BeginWriteAccess() override;
  void EndWriteAccess() override;
  bool BeginReadAccess() override;
  void EndReadAccess() override;
  D3D11TextureAndArrayIndex GetD3D11Texture() const override;

  Microsoft::WRL::ComPtr<ID3D11Texture2D> d3d11_texture_;
};


}  // namespace gpu
#endif  // GPU_COMMAND_BUFFER_SERVICE_SHARED_IMAGE_D3D_IMAGE_REPRESENTATION_H_
