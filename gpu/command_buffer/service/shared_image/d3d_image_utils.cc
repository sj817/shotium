// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image/d3d_image_utils.h"


#include "base/logging.h"
#include "base/notreached.h"
#include "base/strings/stringprintf.h"
#include "gpu/command_buffer/common/shared_image_usage.h"
#include "gpu/config/gpu_finch_features.h"


namespace gpu {

bool ClearD3D11TextureToColor(
    const Microsoft::WRL::ComPtr<ID3D11Texture2D>& d3d11_texture,
    const SkColor4f& color) {
  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device;
  d3d11_texture->GetDevice(&d3d11_device);

  Microsoft::WRL::ComPtr<ID3D11RenderTargetView> render_target;
  HRESULT hr = d3d11_device->CreateRenderTargetView(d3d11_texture.Get(),
                                                    nullptr, &render_target);
  if (FAILED(hr)) {
    LOG(ERROR) << "CreateRenderTargetView failed: "
               << logging::SystemErrorCodeToString(hr);
    return false;
  }
  DCHECK(render_target);

  Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_device_context;
  d3d11_device->GetImmediateContext(&d3d11_device_context);
  DCHECK(d3d11_device_context);

  d3d11_device_context->ClearRenderTargetView(render_target.Get(), color.vec());

  return true;
}








std::string D3D11TextureDescToString(const D3D11_TEXTURE2D_DESC& desc) {
  return base::StringPrintf(
      "width=%u,height=%u,miplevels=%u,arraysize=%u,format=%u,samplecount=%u,"
      "samplequality=%u,usage=%u,bindflags=%08x,cpuaccessflags=%08x,"
      "miscflags=%08x",
      desc.Width, desc.Height, desc.MipLevels, desc.ArraySize, desc.Format,
      desc.SampleDesc.Count, desc.SampleDesc.Quality, desc.Usage,
      desc.BindFlags, desc.CPUAccessFlags, desc.MiscFlags);
}

}  // namespace gpu
