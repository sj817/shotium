// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/shared_image/shared_image_format_service_utils.h"

#include "base/check.h"
#include "base/check_op.h"
#include "base/logging.h"
#include "base/notreached.h"
#include "build/buildflag.h"
#include "components/viz/common/resources/shared_image_format_utils.h"
#include "gpu/command_buffer/service/feature_info.h"
#include "gpu/vulkan/vulkan_ycbcr_info.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_version_info.h"

#if 0
#include "third_party/skia/include/gpu/graphite/dawn/DawnTypes.h"
#endif

namespace gpu {

using PlaneConfig = viz::SharedImageFormat::PlaneConfig;
using ChannelFormat = viz::SharedImageFormat::ChannelFormat;
using Subsampling = viz::SharedImageFormat::Subsampling;

namespace {

#if BUILDFLAG(ENABLE_VULKAN)
VkFormat ToVkFormatSinglePlanarInternal(viz::SharedImageFormat format) {
  CHECK(format.is_single_plane());
  if (format == viz::SinglePlaneFormat::kRGBA_8888) {
    return VK_FORMAT_R8G8B8A8_UNORM;  // or VK_FORMAT_R8G8B8A8_SRGB
  } else if (format == viz::SinglePlaneFormat::kRGBA_4444) {
    return VK_FORMAT_R4G4B4A4_UNORM_PACK16;
  } else if (format == viz::SinglePlaneFormat::kBGRA_8888) {
    return VK_FORMAT_B8G8R8A8_UNORM;
  } else if (format == viz::SinglePlaneFormat::kR_8) {
    return VK_FORMAT_R8_UNORM;
  } else if (format == viz::SinglePlaneFormat::kBGR_565) {
    return VK_FORMAT_R5G6B5_UNORM_PACK16;
  } else if (format == viz::SinglePlaneFormat::kRG_88) {
    return VK_FORMAT_R8G8_UNORM;
  } else if (format == viz::SinglePlaneFormat::kRGBA_F16) {
    return VK_FORMAT_R16G16B16A16_SFLOAT;
  } else if (format == viz::SinglePlaneFormat::kR_16) {
    return VK_FORMAT_R16_UNORM;
  } else if (format == viz::SinglePlaneFormat::kRG_1616) {
    return VK_FORMAT_R16G16_UNORM;
  } else if (format == viz::SinglePlaneFormat::kRGBX_8888) {
    return VK_FORMAT_R8G8B8A8_UNORM;
  } else if (format == viz::SinglePlaneFormat::kBGRX_8888) {
    return VK_FORMAT_B8G8R8A8_UNORM;
  } else if (format == viz::SinglePlaneFormat::kRGBA_1010102) {
    return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
  } else if (format == viz::SinglePlaneFormat::kBGRA_1010102) {
    return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
  } else if (format == viz::SinglePlaneFormat::kALPHA_8) {
    return VK_FORMAT_R8_UNORM;
  } else if (format == viz::SinglePlaneFormat::kETC1) {
    return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
  } else if (format == viz::SinglePlaneFormat::kLUMINANCE_F16 ||
             format == viz::SinglePlaneFormat::kR_F16) {
    return VK_FORMAT_R16_SFLOAT;
  }
  return VK_FORMAT_UNDEFINED;
}
#endif

// Returns GL data format for given `format`.
GLenum GLDataFormat(viz::SharedImageFormat format, int plane_index) {
  DCHECK(format.IsValidPlaneIndex(plane_index));
  if (format.is_single_plane()) {
    if (format == viz::SinglePlaneFormat::kRGBA_8888 ||
        format == viz::SinglePlaneFormat::kRGBA_4444 ||
        format == viz::SinglePlaneFormat::kRGBA_F16 ||
        format == viz::SinglePlaneFormat::kRGBA_1010102 ||
        format == viz::SinglePlaneFormat::kBGRA_1010102) {
      return GL_RGBA;
    } else if (format == viz::SinglePlaneFormat::kBGRA_8888) {
      return GL_BGRA_EXT;
    } else if (format == viz::SinglePlaneFormat::kALPHA_8) {
      return GL_ALPHA;
    } else if (format == viz::SinglePlaneFormat::kLUMINANCE_F16) {
      return GL_LUMINANCE;
    } else if (format == viz::SinglePlaneFormat::kBGR_565 ||
               format == viz::SinglePlaneFormat::kETC1 ||
               format == viz::SinglePlaneFormat::kRGBX_8888 ||
               format == viz::SinglePlaneFormat::kBGRX_8888) {
      return GL_RGB;
    } else if (format == viz::SinglePlaneFormat::kR_8 ||
               format == viz::SinglePlaneFormat::kR_16 ||
               format == viz::SinglePlaneFormat::kR_F16) {
      return GL_RED_EXT;
    } else if (format == viz::SinglePlaneFormat::kRG_88 ||
               format == viz::SinglePlaneFormat::kRG_1616) {
      return GL_RG_EXT;
    }

    return GL_ZERO;
  }

  // For multiplanar formats without external sampler, GL formats are per
  // plane. For single channel planes Y, U, V, A return GL_RED_EXT. For 2
  // channel plane UV return GL_RG_EXT.
  int num_channels = format.NumChannelsInPlane(plane_index);
  DCHECK_LE(num_channels, 2);
  return num_channels == 2 ? GL_RG_EXT : GL_RED_EXT;
}

// Returns GL data type for given `format`.
GLenum GLDataType(viz::SharedImageFormat format) {
  if (format.is_single_plane()) {
    if (format == viz::SinglePlaneFormat::kRGBA_8888 ||
        format == viz::SinglePlaneFormat::kBGRA_8888 ||
        format == viz::SinglePlaneFormat::kALPHA_8 ||
        format == viz::SinglePlaneFormat::kETC1 ||
        format == viz::SinglePlaneFormat::kR_8 ||
        format == viz::SinglePlaneFormat::kRG_88 ||
        format == viz::SinglePlaneFormat::kRGBX_8888 ||
        format == viz::SinglePlaneFormat::kBGRX_8888) {
      return GL_UNSIGNED_BYTE;
    } else if (format == viz::SinglePlaneFormat::kRGBA_4444) {
      return GL_UNSIGNED_SHORT_4_4_4_4;
    } else if (format == viz::SinglePlaneFormat::kBGR_565) {
      return GL_UNSIGNED_SHORT_5_6_5;
    } else if (format == viz::SinglePlaneFormat::kLUMINANCE_F16 ||
               format == viz::SinglePlaneFormat::kR_F16 ||
               format == viz::SinglePlaneFormat::kRGBA_F16) {
      return GL_HALF_FLOAT_OES;
    } else if (format == viz::SinglePlaneFormat::kR_16 ||
               format == viz::SinglePlaneFormat::kRG_1616) {
      return GL_UNSIGNED_SHORT;
    } else if (format == viz::SinglePlaneFormat::kRGBA_1010102 ||
               format == viz::SinglePlaneFormat::kBGRA_1010102) {
      return GL_UNSIGNED_INT_2_10_10_10_REV_EXT;
    }

    return GL_ZERO;
  }

  switch (format.channel_format()) {
    case ChannelFormat::k8:
      return GL_UNSIGNED_BYTE;
    case ChannelFormat::k10:
      return GL_UNSIGNED_SHORT;
    case ChannelFormat::k16:
      return GL_UNSIGNED_SHORT;
    case ChannelFormat::k16F:
      return GL_HALF_FLOAT_OES;
  }
}

// Returns the GL format used internally for matching with the texture format
// for a given `format`.
GLenum GLInternalFormat(viz::SharedImageFormat format, int plane_index) {
  DCHECK(format.IsValidPlaneIndex(plane_index));
  if (format.is_single_plane()) {
    // In GLES2, the internal format must match the texture format. (It no
    // longer is true in GLES3, however it still holds for the BGRA
    // extension.) GL_EXT_texture_norm16 follows GLES3 semantics and only
    // exposes a sized internal format (GL_R16_EXT).
    if (format == viz::SinglePlaneFormat::kR_16) {
      return GL_R16_EXT;
    } else if (format == viz::SinglePlaneFormat::kRG_1616) {
      return GL_RG16_EXT;
    } else if (format == viz::SinglePlaneFormat::kETC1) {
      return GL_ETC1_RGB8_OES;
    } else if (format == viz::SinglePlaneFormat::kRGBA_1010102 ||
               format == viz::SinglePlaneFormat::kBGRA_1010102) {
      return GL_RGB10_A2_EXT;
    }
    return GLDataFormat(format, /*plane_index=*/0);
  }

  // For multiplanar formats without external sampler, GL formats are per
  // plane. For single channel 8-bit planes Y, U, V, A return GL_RED_EXT. For
  // single channel 10/16-bit planes Y,  U, V, A return GL_R16_EXT. For 2
  // channel plane 8-bit UV return GL_RG_EXT. For 2 channel plane 10/16-bit UV
  // return GL_RG16_EXT.
  int num_channels = format.NumChannelsInPlane(plane_index);
  DCHECK_LE(num_channels, 2);
  switch (format.channel_format()) {
    case ChannelFormat::k8:
      return num_channels == 2 ? GL_RG_EXT : GL_RED_EXT;
    case ChannelFormat::k10:
    case ChannelFormat::k16:
      return num_channels == 2 ? GL_RG16_EXT : GL_R16_EXT;
    case ChannelFormat::k16F:
      return num_channels == 2 ? GL_RG16F_EXT : GL_R16F_EXT;
  }
}

// Returns texture storage format for given `format`.
GLenum TextureStorageFormat(viz::SharedImageFormat format,
                            int plane_index,
                            bool use_angle_rgbx_format) {
  DCHECK(format.IsValidPlaneIndex(plane_index));
  if (format.is_single_plane()) {
    if (format == viz::SinglePlaneFormat::kRGBA_8888) {
      return GL_RGBA8_OES;
    } else if (format == viz::SinglePlaneFormat::kBGRA_8888) {
      return GL_BGRA8_EXT;
    } else if (format == viz::SinglePlaneFormat::kRGBA_F16) {
      return GL_RGBA16F_EXT;
    } else if (format == viz::SinglePlaneFormat::kRGBA_4444) {
      return GL_RGBA4;
    } else if (format == viz::SinglePlaneFormat::kALPHA_8) {
      return GL_ALPHA8_EXT;
    } else if (format == viz::SinglePlaneFormat::kBGR_565) {
      return GL_RGB565;
    } else if (format == viz::SinglePlaneFormat::kR_8) {
      return GL_R8_EXT;
    } else if (format == viz::SinglePlaneFormat::kRG_88) {
      return GL_RG8_EXT;
    } else if (format == viz::SinglePlaneFormat::kLUMINANCE_F16) {
      return GL_LUMINANCE16F_EXT;
    } else if (format == viz::SinglePlaneFormat::kR_F16) {
      return GL_R16F_EXT;
    } else if (format == viz::SinglePlaneFormat::kR_16) {
      return GL_R16_EXT;
    } else if (format == viz::SinglePlaneFormat::kRG_1616) {
      return GL_RG16_EXT;
    } else if (format == viz::SinglePlaneFormat::kRGBX_8888 ||
               format == viz::SinglePlaneFormat::kBGRX_8888) {
      return use_angle_rgbx_format ? GL_RGBX8_ANGLE : GL_RGB8_OES;
    } else if (format == viz::SinglePlaneFormat::kETC1) {
      return GL_ETC1_RGB8_OES;
    } else if (format == viz::SinglePlaneFormat::kRGBA_1010102 ||
               format == viz::SinglePlaneFormat::kBGRA_1010102) {
      return GL_RGB10_A2_EXT;
    }

    NOTREACHED();
  }

  // For multiplanar formats without external sampler, GL formats are per
  // plane. For single channel 8-bit planes Y, U, V, A return GL_R8_EXT. For
  // single channel 10/16-bit planes Y,  U, V, A return GL_R16_EXT. For 2
  // channel plane 8-bit UV return GL_RG8_EXT. For 2 channel plane 10/16-bit
  // UV return GL_RG16_EXT.
  int num_channels = format.NumChannelsInPlane(plane_index);
  DCHECK_LE(num_channels, 2);
  switch (format.channel_format()) {
    case ChannelFormat::k8:
      return num_channels == 2 ? GL_RG8_EXT : GL_R8_EXT;
    case ChannelFormat::k10:
    case ChannelFormat::k16:
      return num_channels == 2 ? GL_RG16_EXT : GL_R16_EXT;
    case ChannelFormat::k16F:
      return num_channels == 2 ? GL_RG16F_EXT : GL_R16F_EXT;
  }
}
}  // namespace

bool IsSizeForBufferHandleValid(const gfx::Size& size,
                                viz::SharedImageFormat format) {
  if (format.is_single_plane()) {
    return true;
  }

#if BUILDFLAG(IS_CHROMEOS)
  // Allow odd size for CrOS.
  // TODO(https://crbug.com/1208788, https://crbug.com/1224781): Merge this
  // with the path that uses viz::IsOddSizeMultiPlanarBuffersAllowed.
  return true;
#else
  auto [width_scale, height_scale] = format.GetSubsamplingScale();
  if (size.width() % width_scale &&
      !viz::IsOddSizeMultiPlanarBuffersAllowed()) {
    return false;
  }
  if (size.height() % height_scale &&
      !viz::IsOddSizeMultiPlanarBuffersAllowed()) {
    return false;
  }
  return true;
#endif  // BUILDFLAG(IS_CHROMEOS)
}

SkYUVAInfo::PlaneConfig ToSkYUVAPlaneConfig(viz::SharedImageFormat format) {
  switch (format.plane_config()) {
    case PlaneConfig::kY_U_V:
      return SkYUVAInfo::PlaneConfig::kY_U_V;
    case PlaneConfig::kY_V_U:
      return SkYUVAInfo::PlaneConfig::kY_V_U;
    case PlaneConfig::kY_UV:
      return SkYUVAInfo::PlaneConfig::kY_UV;
    case PlaneConfig::kY_UV_A:
      return SkYUVAInfo::PlaneConfig::kY_UV_A;
    case PlaneConfig::kY_U_V_A:
      return SkYUVAInfo::PlaneConfig::kY_U_V_A;
  }
}

SkYUVAInfo::Subsampling ToSkYUVASubsampling(viz::SharedImageFormat format) {
  switch (format.subsampling()) {
    case Subsampling::k420:
      return SkYUVAInfo::Subsampling::k420;
    case Subsampling::k422:
      return SkYUVAInfo::Subsampling::k422;
    case Subsampling::k444:
      return SkYUVAInfo::Subsampling::k444;
  }
}

SkColorType ToClosestSkColorTypeExternalSampler(viz::SharedImageFormat format) {
  CHECK(format.PrefersExternalSampler());
  auto channel_format = format.channel_format();
  switch (channel_format) {
    case ChannelFormat::k8:
      return format.HasAlpha() ? kRGBA_8888_SkColorType : kRGB_888x_SkColorType;
    case ChannelFormat::k10:
      return kRGBA_1010102_SkColorType;
    case ChannelFormat::k16:
      return kR16G16B16A16_unorm_SkColorType;
    case ChannelFormat::k16F:
      return kRGBA_F16_SkColorType;
  }
}

GLFormatCaps::GLFormatCaps(const gles2::FeatureInfo* feature_info)
    : angle_rgbx_internal_format_(
          feature_info->feature_flags().angle_rgbx_internal_format),
      oes_texture_float_available_(feature_info->oes_texture_float_available()),
      ext_texture_rg_(feature_info->feature_flags().ext_texture_rg),
      ext_texture_norm16_(feature_info->feature_flags().ext_texture_norm16),
      enable_texture_half_float_linear_(
          feature_info->feature_flags().enable_texture_half_float_linear),
      is_atleast_gles3_(feature_info->gl_version_info().IsAtLeastGLES(3, 0)) {}

GLFormatDesc GLFormatCaps::ToGLFormatDescExternalSampler(
    viz::SharedImageFormat format) const {
  CHECK(format.PrefersExternalSampler());
  GLenum ext_format = format.HasAlpha() ? GL_RGBA : GL_RGB;
  GLFormatDesc gl_format;
  gl_format.data_type = GL_NONE;
  gl_format.data_format = ext_format;
  gl_format.image_internal_format = ext_format;
  switch (format.channel_format()) {
    case ChannelFormat::k8:
      gl_format.storage_internal_format =
          format.HasAlpha() ? GL_RGBA8_OES : GL_RGB8_OES;
      break;
    case ChannelFormat::k10:
      gl_format.storage_internal_format = GL_RGB10_A2_EXT;
      break;
    case ChannelFormat::k16:
      gl_format.storage_internal_format = GL_RGBA16_EXT;
      break;
    case ChannelFormat::k16F:
      gl_format.storage_internal_format = GL_RGBA16F_EXT;
      break;
  }
  gl_format.target = GL_TEXTURE_EXTERNAL_OES;
  return gl_format;
}

GLFormatDesc GLFormatCaps::ToGLFormatDesc(viz::SharedImageFormat format,
                                          int plane_index) const {
  GLFormatDesc gl_format;
  gl_format.data_type = GLDataType(format);
  gl_format.data_format = GLDataFormat(format, plane_index);
  gl_format.image_internal_format = GLInternalFormat(format, plane_index);
  gl_format.storage_internal_format =
      TextureStorageFormat(format, plane_index, angle_rgbx_internal_format_);
  if (format.is_multi_plane()) {
    gl_format.data_format =
        GetFallbackFormatIfNotSupported(gl_format.data_format);
    gl_format.image_internal_format =
        GetFallbackFormatIfNotSupported(gl_format.image_internal_format);
    gl_format.storage_internal_format =
        GetFallbackFormatIfNotSupported(gl_format.storage_internal_format);
  }
  gl_format.target = GL_TEXTURE_2D;
  return gl_format;
}

GLFormatDesc GLFormatCaps::ToGLFormatDescOverrideHalfFloatType(
    viz::SharedImageFormat format,
    int plane_index) const {
  GLFormatDesc format_desc = ToGLFormatDesc(format, plane_index);
  // GL_HALF_FLOAT and GL_HALF_FLOAT_OES have different values so cannot be used
  // interchangeably.
  if (format_desc.data_type == GL_HALF_FLOAT_OES &&
      !oes_texture_float_available_) {
    format_desc.data_type = GL_HALF_FLOAT;
  }
  // ES3 requires using sized internal format for GL_HALF_FLOAT.
  if (format_desc.image_internal_format == GL_RGBA &&
      format_desc.data_format == GL_RGBA &&
      format_desc.data_type == GL_HALF_FLOAT) {
    format_desc.image_internal_format = GL_RGBA16F;
  }
  return format_desc;
}

GLenum GLFormatCaps::GetFallbackFormatIfNotSupported(GLenum gl_format) const {
  // Fallback to GL_ALPHA for unsized RED format.
  if (gl_format == GL_RED_EXT && !ext_texture_rg_) {
    return GL_ALPHA;
  }
  // Fallback to GL_ALPHA8 for sized R8 format.
  if (gl_format == GL_R8_EXT && !ext_texture_rg_) {
    return GL_ALPHA8_EXT;
  }
  // No fallback for sized/unsize RG8 format without texture_rg extension.
  if ((gl_format == GL_RG_EXT || gl_format == GL_RG8_EXT) && !ext_texture_rg_) {
    return GL_ZERO;
  }
  // No fallback for R16, RG16 format without texture_norm16 extension.
  if ((gl_format == GL_R16_EXT || gl_format == GL_RG16_EXT) &&
      !ext_texture_norm16_) {
    return GL_ZERO;
  }
  // Fallback to GL_LUMINANCE16F for R16F format based on extensions and ES3
  // support.
  if (gl_format == GL_R16F_EXT &&
      (!is_atleast_gles3_ && !enable_texture_half_float_linear_)) {
    return GL_LUMINANCE16F_EXT;
  }
  // No fallback for RG16F format without texture_rg extension.
  if (gl_format == GL_RG16F_EXT && !ext_texture_rg_) {
    return GL_ZERO;
  }
  // Return original format if its supported.
  return gl_format;
}

#if BUILDFLAG(ENABLE_VULKAN)
bool HasVkFormat(viz::SharedImageFormat format) {
  if (format.is_single_plane()) {
    return ToVkFormatSinglePlanarInternal(format) != VK_FORMAT_UNDEFINED;
  }
  if (format.PrefersExternalSampler()) {
    return ToVkFormatExternalSampler(format) != VK_FORMAT_UNDEFINED;
  }
  for (int plane = 0; plane < format.NumberOfPlanes(); plane++) {
    if (ToVkFormat(format, plane) == VK_FORMAT_UNDEFINED) {
      return false;
    }
  }
  return true;
}

VkFormat ToVkFormatExternalSampler(viz::SharedImageFormat format) {
  CHECK(format.PrefersExternalSampler());

  // Return early for unsupported kY_UV_A plane configs.
  if (format.plane_config() == PlaneConfig::kY_UV_A) {
    return VK_FORMAT_UNDEFINED;
  }

  switch (format.channel_format()) {
    case ChannelFormat::k8:
      return format.plane_config() == PlaneConfig::kY_UV
                 ? VK_FORMAT_G8_B8R8_2PLANE_420_UNORM
                 : VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM;
    case ChannelFormat::k10:
      return format.plane_config() == PlaneConfig::kY_UV
                 ? VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16
                 : VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16;
    case ChannelFormat::k16:
      return format.plane_config() == PlaneConfig::kY_UV
                 ? VK_FORMAT_G16_B16R16_2PLANE_420_UNORM
                 : VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM;
    case ChannelFormat::k16F:
      return VK_FORMAT_UNDEFINED;
  }
}

VkFormat ToVkFormatSinglePlanar(viz::SharedImageFormat format) {
  CHECK(format.is_single_plane());
  auto result = ToVkFormatSinglePlanarInternal(format);
  DCHECK_NE(result, VK_FORMAT_UNDEFINED)
      << "Unsupported format " << format.ToString();
  return result;
}

VkFormat ToVkFormat(viz::SharedImageFormat format, int plane_index) {
  DCHECK(format.IsValidPlaneIndex(plane_index));

  if (format.is_single_plane()) {
    return ToVkFormatSinglePlanar(format);
  }

  // Since the format has PrefersExternalSampler() false we create a separate
  // VkImage per plane and return the single planar equivalents. NOTE: Callsites
  // that handle formats with external sampling need to call
  // ToVkFormatExternalSampler() if external sampling is being used.
  CHECK(!format.PrefersExternalSampler());
  int num_channels = format.NumChannelsInPlane(plane_index);
  CHECK_LE(num_channels, 2);
  switch (format.channel_format()) {
    case ChannelFormat::k8:
      return num_channels == 2 ? VK_FORMAT_R8G8_UNORM : VK_FORMAT_R8_UNORM;
    case ChannelFormat::k10:
    case ChannelFormat::k16:
      return num_channels == 2 ? VK_FORMAT_R16G16_UNORM : VK_FORMAT_R16_UNORM;
    case ChannelFormat::k16F:
      return num_channels == 2 ? VK_FORMAT_R16G16_SFLOAT : VK_FORMAT_R16_SFLOAT;
  }
}
#endif

#if BUILDFLAG(IS_WIN)
// Formats supported with no GpuMemoryBufferHandle.
DXGI_FORMAT ToDXGIFormat(viz::SharedImageFormat format) {
  if (format == viz::SinglePlaneFormat::kRGBA_F16) {
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
  } else if (format == viz::SinglePlaneFormat::kBGRA_8888 ||
             format == viz::SinglePlaneFormat::kBGRX_8888) {
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  } else if (format == viz::SinglePlaneFormat::kRGBA_8888 ||
             format == viz::SinglePlaneFormat::kRGBX_8888) {
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  } else if (format == viz::MultiPlaneFormat::kNV12) {
    return DXGI_FORMAT_NV12;
  } else if (format == viz::SinglePlaneFormat::kRGBA_1010102) {
    return DXGI_FORMAT_R10G10B10A2_UNORM;
  } else if (format == viz::SinglePlaneFormat::kR_8) {
    // TOOD(crbug.com/416285370): Remove these single channel format checks.
    return DXGI_FORMAT_R8_UNORM;
  } else if (format == viz::SinglePlaneFormat::kRG_88) {
    return DXGI_FORMAT_R8G8_UNORM;
  } else if (format == viz::SinglePlaneFormat::kR_16) {
    return DXGI_FORMAT_R16_UNORM;
  } else if (format == viz::SinglePlaneFormat::kRG_1616) {
    return DXGI_FORMAT_R16G16_UNORM;
  }
  return DXGI_FORMAT_UNKNOWN;
}
#endif  // BUILDFLAG(IS_WIN)





skgpu::graphite::TextureInfo GraphiteBackendTextureInfo(
    GrContextType gr_context_type,
    viz::SharedImageFormat format,
    int plane_index,
    bool is_yuv_plane,
    bool mipmapped,
    bool scanout_dcomp_surface) {
  // This used to build a skgpu::graphite::DawnTextureInfo; Graphite in this
  // codebase was only ever backed by Dawn, and Dawn is gone. Return a
  // default (invalid) TextureInfo so callers' existing isValid() checks
  // fail gracefully instead of crashing.
  return skgpu::graphite::TextureInfo();
}

skgpu::graphite::TextureInfo GraphitePromiseTextureInfo(
    GrContextType gr_context_type,
    viz::SharedImageFormat format,
    std::optional<VulkanYCbCrInfo> ycbcr_info,
    int plane_index,
    bool mipmapped) {
  // See GraphiteBackendTextureInfo() above: Graphite was only ever
  // Dawn-backed here, and Dawn is gone.
  return skgpu::graphite::TextureInfo();
}

// DawnBackendTextureInfo() (built a skgpu::graphite::DawnTextureInfo) and the
// Dawn-specific branch of FallbackGraphiteBackendTextureInfo() are gone with
// Dawn.

skgpu::graphite::TextureInfo FallbackGraphiteBackendTextureInfo(
    const skgpu::graphite::TextureInfo& texture_info) {
  return texture_info;
}

}  // namespace gpu
