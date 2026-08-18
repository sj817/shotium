// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/config/gpu_info_collector.h"

#include <stddef.h>
#include <stdint.h>

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/metrics/histogram_functions.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "gpu/config/gpu_switches.h"
#include "skia/buildflags.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"
#include "third_party/angle/src/gpu_info_util/SystemInfo.h"  // nogncheck
#include "third_party/skia/include/core/SkGraphics.h"
#include "ui/gl/buildflags.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_context.h"
#include "ui/gl/gl_features.h"
#include "ui/gl/gl_implementation.h"
#include "ui/gl/gl_surface.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/gl/gl_switches.h"
#include "ui/gl/gl_utils.h"
#include "ui/gl/gl_version_info.h"
#include "ui/gl/init/create_gr_gl_interface.h"
#include "ui/gl/init/gl_factory.h"

#if BUILDFLAG(IS_MAC)
#include "base/apple/bundle_locations.h"
#include "base/apple/foundation_util.h"
#endif

#if BUILDFLAG(IS_OZONE)
#include "ui/ozone/public/ozone_platform.h"           // nogncheck
#include "ui/ozone/public/platform_gl_egl_utility.h"  // nogncheck
#endif

namespace {

// From ANGLE's egl/eglext.h.
#ifndef EGL_ANGLE_feature_control
#define EGL_ANGLE_feature_control 1
#define EGL_FEATURE_NAME_ANGLE 0x3460
#define EGL_FEATURE_CATEGORY_ANGLE 0x3461
#define EGL_FEATURE_STATUS_ANGLE 0x3464
#define EGL_FEATURE_COUNT_ANGLE 0x3465
#define EGL_FEATURE_OVERRIDES_ENABLED_ANGLE 0x3466
#define EGL_FEATURE_OVERRIDES_DISABLED_ANGLE 0x3467
#endif /* EGL_ANGLE_feature_control */

scoped_refptr<gl::GLSurface> InitializeGLSurface(gl::GLDisplay* display) {
  scoped_refptr<gl::GLSurface> surface(
      gl::init::CreateOffscreenGLSurface(display, gfx::Size()));
  if (!surface.get()) {
    LOG(ERROR) << "gl::GLContext::CreateOffscreenGLSurface failed";
    return nullptr;
  }

  return surface;
}

scoped_refptr<gl::GLContext> InitializeGLContext(gl::GLSurface* surface) {
  TRACE_EVENT("gpu,startup", "gpu_info_collector::InitializeGLContext");
  gl::GLContextAttribs attribs;
  attribs.allow_es_version_fallback =
      !features::ShouldFallbackToSWIfGLES3NotSupported();
  scoped_refptr<gl::GLContext> context(
      gl::init::CreateGLContext(nullptr, surface, attribs));
  if (!context.get()) {
    LOG(ERROR) << "gl::init::CreateGLContext failed";
    return nullptr;
  }

  if (!context->MakeCurrent(surface)) {
    LOG(ERROR) << "gl::GLContext::MakeCurrent() failed";
    return nullptr;
  }

  return context;
}

std::string GetGLString(unsigned int pname) {
  const char* gl_string = reinterpret_cast<const char*>(glGetString(pname));
  if (gl_string)
    return std::string(gl_string);
  return std::string();
}

std::string QueryEGLStringi(EGLDisplay display,
                            unsigned int name,
                            unsigned int index) {
  const char* egl_string =
      reinterpret_cast<const char*>(eglQueryStringiANGLE(display, name, index));
  if (egl_string)
    return std::string(egl_string);
  return std::string();
}

// Return a version string in the format of "major.minor".
std::string GetVersionFromString(const std::string& version_string) {
  size_t begin = version_string.find_first_of("0123456789");
  if (begin != std::string::npos) {
    size_t end = version_string.find_first_not_of("01234567890.", begin);
    std::string sub_string;
    if (end != std::string::npos)
      sub_string = version_string.substr(begin, end - begin);
    else
      sub_string = version_string.substr(begin);
    std::vector<std::string> pieces = base::SplitString(
        sub_string, ".", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    if (pieces.size() >= 2)
      return pieces[0] + "." + pieces[1];
  }
  return std::string();
}

// Return the array index of the found name, or return -1.
int StringContainsName(const std::string& str,
                       base::span<const std::string> names) {
  std::vector<std::string> tokens = base::SplitString(
      str, " .,()-_", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
  for (size_t ii = 0; ii < tokens.size(); ++ii) {
    for (size_t name_index = 0; name_index < names.size(); ++name_index) {
      if (tokens[ii] == names[name_index]) {
        return base::checked_cast<int>(name_index);
      }
    }
  }
  return -1;
}

std::string GetDisplayTypeString(gl::DisplayType type) {
  switch (type) {
    case gl::DEFAULT:
      return "DEFAULT";
    case gl::SWIFT_SHADER:
      return "SWIFT_SHADER";
    case gl::ANGLE_WARP:
      return "ANGLE_WARP";
    case gl::ANGLE_D3D11:
      return "ANGLE_D3D11";
    case gl::ANGLE_OPENGL:
      return "ANGLE_OPENGL";
    case gl::ANGLE_OPENGLES:
      return "ANGLE_OPENGLES";
    case gl::ANGLE_NULL:
      return "ANGLE_NULL";
    case gl::ANGLE_D3D11_NULL:
      return "ANGLE_D3D11_NULL";
    case gl::ANGLE_OPENGL_NULL:
      return "ANGLE_OPENGL_NULL";
    case gl::ANGLE_OPENGLES_NULL:
      return "ANGLE_OPENGLES_NULL";
    case gl::ANGLE_VULKAN:
      return "ANGLE_VULKAN";
    case gl::ANGLE_VULKAN_NULL:
      return "ANGLE_VULKAN_NULL";
    case gl::ANGLE_D3D11on12:
      return "ANGLE_D3D11on12";
    case gl::ANGLE_SWIFTSHADER:
      return "ANGLE_SWIFTSHADER";
    case gl::ANGLE_OPENGL_EGL:
      return "ANGLE_OPENGL_EGL";
    case gl::ANGLE_OPENGLES_EGL:
      return "ANGLE_OPENGLES_EGL";
    case gl::ANGLE_METAL:
      return "ANGLE_METAL";
    case gl::ANGLE_METAL_NULL:
      return "ANGLE_METAL_NULL";
    case gl::ANGLE_D3D11_WARP:
      return "ANGLE_D3D11_WARP";
    default:
      NOTREACHED();
  }
}


}  // namespace

namespace gpu {

bool CollectGraphicsDeviceInfoFromCommandLine(
    const base::CommandLine* command_line,
    GPUInfo* gpu_info) {
  GPUInfo::GPUDevice& gpu = gpu_info->gpu;

  if (command_line->HasSwitch(switches::kGpuVendorId)) {
    const std::string vendor_id_str =
        command_line->GetSwitchValueASCII(switches::kGpuVendorId);
    base::StringToUint(vendor_id_str, &gpu.vendor_id);
  }

  if (command_line->HasSwitch(switches::kGpuDeviceId)) {
    const std::string device_id_str =
        command_line->GetSwitchValueASCII(switches::kGpuDeviceId);
    base::StringToUint(device_id_str, &gpu.device_id);
  }

#if BUILDFLAG(IS_WIN)
  if (command_line->HasSwitch(switches::kGpuSubSystemId)) {
    const std::string syb_system_id_str =
        command_line->GetSwitchValueASCII(switches::kGpuSubSystemId);
    base::StringToUint(syb_system_id_str, &gpu.sub_sys_id);
  }

  if (command_line->HasSwitch(switches::kGpuRevision)) {
    const std::string revision_str =
        command_line->GetSwitchValueASCII(switches::kGpuRevision);
    base::StringToUint(revision_str, &gpu.revision);
  }
#endif

  if (command_line->HasSwitch(switches::kGpuDriverVersion)) {
    gpu.driver_version =
        command_line->GetSwitchValueASCII(switches::kGpuDriverVersion);
  }

  bool info_updated = gpu.vendor_id || gpu.device_id ||
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_CHROMEOS)
                      gpu.revision ||
#endif
#if BUILDFLAG(IS_WIN)
                      gpu.sub_sys_id ||
#endif
                      !gpu.driver_version.empty();

  return info_updated;
}

bool CollectBasicGraphicsInfo(const base::CommandLine* command_line,
                              GPUInfo* gpu_info) {
  TRACE_EVENT("gpu,startup", "gpu_info_collector::CollectBasicGraphicsInfo");
  // In the info-collection GPU process on Windows, we get the device info from
  // the browser.
  if (CollectGraphicsDeviceInfoFromCommandLine(command_line, gpu_info)) {
    return true;
  }

  // We can't check if passthrough is supported yet because GL may not be
  // initialized.
  gpu_info->passthrough_cmd_decoder =
      gl::UsePassthroughCommandDecoder(command_line);

  std::optional<gl::GLImplementationParts> implementation =
      gl::GetRequestedGLImplementationFromCommandLine(command_line);

  if (implementation == gl::kGLImplementationDisabled) {
    // If GL is disabled then we don't need GPUInfo.
    gpu_info->gl_vendor = "Disabled";
    gpu_info->gl_renderer = "Disabled";
    gpu_info->gl_version = "Disabled";
    return true;
  } else if (implementation == gl::GetSoftwareGLImplementation(command_line)) {
    // If using the software GL implementation, use fake vendor and
    // device ids to make sure it never gets blocklisted. It allows us
    // to proceed with loading the blocklist which may have non-device
    // specific entries we want to apply anyways (e.g., OS version
    // blocklisting).
    gpu_info->gpu.vendor_id = 0xffff;
    gpu_info->gpu.device_id = 0xffff;

    // Also declare the driver_vendor to be <SwANGLE> to be able to
    // specify exceptions based on driver_vendor==<SwANGLE> for some
    // blocklist rules.
    gpu_info->gpu.driver_vendor = "SwANGLE";

    GPUInfo swangle_gpu_info(*gpu_info);
    if (CollectBasicGraphicsInfo(&swangle_gpu_info)) {
      // Also store the machine model and version
      gpu_info->machine_model_name = swangle_gpu_info.machine_model_name;
      gpu_info->machine_model_version = swangle_gpu_info.machine_model_version;
    }

    return true;
  }

  return CollectBasicGraphicsInfo(gpu_info);
}

bool CollectGraphicsInfoGL(GPUInfo* gpu_info, gl::GLDisplay* display) {
  TRACE_EVENT("gpu,startup", "gpu_info_collector::CollectGraphicsInfoGL");
  DCHECK_NE(gl::GetGLImplementationParts(), gl::kGLImplementationNone);
  gl::GLDisplayEGL* egl_display = display->GetAs<gl::GLDisplayEGL>();

  scoped_refptr<gl::GLSurface> surface(InitializeGLSurface(display));
  if (!surface.get()) {
    LOG(ERROR) << "Could not create surface for info collection.";
    return false;
  }

  scoped_refptr<gl::GLContext> context(InitializeGLContext(surface.get()));
  if (!context.get()) {
    LOG(ERROR) << "Could not create context for info collection.";
    return false;
  }

  if (egl_display) {
    gpu_info->display_type =
        GetDisplayTypeString(egl_display->GetDisplayType());
  }
  gpu_info->gl_renderer = GetGLString(GL_RENDERER);
  gpu_info->gl_vendor = GetGLString(GL_VENDOR);
  gpu_info->gl_version = GetGLString(GL_VERSION);
  std::string glsl_version_string = GetGLString(GL_SHADING_LANGUAGE_VERSION);

  gpu_info->gl_extensions = gl::GetGLExtensionsFromCurrentContext();
  gfx::ExtensionSet extension_set =
      gfx::MakeExtensionSet(gpu_info->gl_extensions);

  gl::GLVersionInfo gl_info(gpu_info->gl_version.c_str(),
                            gpu_info->gl_renderer.c_str(), extension_set);
  GPUInfo::GPUDevice& active_gpu = gpu_info->active_gpu();
  if (!gl_info.driver_vendor.empty() && active_gpu.driver_vendor.empty()) {
    active_gpu.driver_vendor = gl_info.driver_vendor;
  }
  if (!gl_info.driver_version.empty() && active_gpu.driver_version.empty()) {
    active_gpu.driver_version = gl_info.driver_version;
  }

  GLint max_samples = 0;
  if (gl_info.IsAtLeastGLES(3, 0) ||
      gfx::HasExtension(extension_set, "GL_ANGLE_framebuffer_multisample") ||
      gfx::HasExtension(extension_set, "GL_APPLE_framebuffer_multisample") ||
      gfx::HasExtension(extension_set, "GL_EXT_framebuffer_multisample") ||
      gfx::HasExtension(extension_set,
                        "GL_EXT_multisampled_render_to_texture") ||
      gfx::HasExtension(extension_set, "GL_NV_framebuffer_multisample")) {
    glGetIntegerv(GL_MAX_SAMPLES, &max_samples);
  }
  gpu_info->max_msaa_samples = base::NumberToString(max_samples);

#if BUILDFLAG(IS_ANDROID)
  gpu_info->can_support_threaded_texture_mailbox =
      egl_display->ext->b_EGL_KHR_fence_sync &&
      egl_display->ext->b_EGL_KHR_image_base &&
      egl_display->ext->b_EGL_KHR_gl_texture_2D_image &&
      gfx::HasExtension(extension_set, "GL_OES_EGL_image");
#else
  gl::GLWindowSystemBindingInfo window_system_binding_info;
  if (gl::init::GetGLWindowSystemBindingInfo(gl_info,
                                             &window_system_binding_info)) {
    gpu_info->gl_ws_vendor = window_system_binding_info.vendor;
    gpu_info->gl_ws_version = window_system_binding_info.version;
    gpu_info->gl_ws_extensions = window_system_binding_info.extensions;
    gpu_info->direct_rendering_version =
        window_system_binding_info.direct_rendering_version;
  }
#endif  // BUILDFLAG(IS_ANDROID)

  bool supports_robustness =
      gfx::HasExtension(extension_set, "GL_EXT_robustness") ||
      gfx::HasExtension(extension_set, "GL_KHR_robustness") ||
      gfx::HasExtension(extension_set, "GL_ARB_robustness");
  if (supports_robustness) {
    glGetIntegerv(
        GL_RESET_NOTIFICATION_STRATEGY,
        reinterpret_cast<GLint*>(&gpu_info->gl_reset_notification_strategy));
  }

  // TODO(kbr): remove once the destruction of a current context automatically
  // clears the current context.
  context->ReleaseCurrent(surface.get());

  std::string glsl_version = GetVersionFromString(glsl_version_string);
  gpu_info->pixel_shader_version = glsl_version;
  gpu_info->vertex_shader_version = glsl_version;

  bool active_gpu_identified = false;
#if BUILDFLAG(IS_WIN)
  active_gpu_identified = IdentifyActiveGPUWithLuid(gpu_info);
#endif  // BUILDFLAG(IS_WIN)

  if (!active_gpu_identified)
    IdentifyActiveGPU(gpu_info);

  return true;
}

void IdentifyActiveGPU(GPUInfo* gpu_info) {
  const std::string kNVidiaName = "nvidia";
  const std::string kNouveauName = "nouveau";
  const std::string kIntelName = "intel";
  const std::string kAMDName = "amd";
  const std::string kATIName = "ati";
  const std::array<std::string, 5> kVendorNames = {
      {kNVidiaName, kNouveauName, kIntelName, kAMDName, kATIName}};

  const uint32_t kNVidiaID = 0x10de;
  const uint32_t kIntelID = 0x8086;
  const uint32_t kAMDID = 0x1002;
  const uint32_t kATIID = 0x1002;
  const std::array<uint32_t, 5> kVendorIDs = {
      {kNVidiaID, kNVidiaID, kIntelID, kAMDID, kATIID}};

  DCHECK(gpu_info);
  if (gpu_info->secondary_gpus.size() == 0) {
    // If there is only a single GPU, that GPU is active.
    gpu_info->gpu.active = true;
    gpu_info->gpu.vendor_string = gpu_info->gl_vendor;
    gpu_info->gpu.device_string = gpu_info->gl_renderer;
    return;
  }

  uint32_t active_vendor_id = 0;
  if (!gpu_info->gl_vendor.empty()) {
    std::string gl_vendor_lower = base::ToLowerASCII(gpu_info->gl_vendor);
    int index = StringContainsName(gl_vendor_lower, kVendorNames);
    if (index >= 0) {
      active_vendor_id = kVendorIDs[index];
    }
  }
  if (active_vendor_id == 0 && !gpu_info->gl_renderer.empty()) {
    std::string gl_renderer_lower = base::ToLowerASCII(gpu_info->gl_renderer);
    int index = StringContainsName(gl_renderer_lower, kVendorNames);
    if (index >= 0) {
      active_vendor_id = kVendorIDs[index];
    }
  }
  if (active_vendor_id == 0) {
    // We fail to identify the GPU vendor through GL_VENDOR/GL_RENDERER.
    return;
  }
  gpu_info->gpu.active = false;
  for (size_t ii = 0; ii < gpu_info->secondary_gpus.size(); ++ii)
    gpu_info->secondary_gpus[ii].active = false;

  // TODO(zmo): if two GPUs are from the same vendor, this code will always
  // set the first GPU as active, which could be wrong.
  if (active_vendor_id == gpu_info->gpu.vendor_id) {
    gpu_info->gpu.active = true;
    return;
  }
  for (size_t ii = 0; ii < gpu_info->secondary_gpus.size(); ++ii) {
    if (active_vendor_id == gpu_info->secondary_gpus[ii].vendor_id) {
      gpu_info->secondary_gpus[ii].active = true;
      return;
    }
  }
}

void FillGPUInfoFromSystemInfo(GPUInfo* gpu_info,
                               angle::SystemInfo* system_info) {
  // We fill gpu_info even when angle::GetSystemInfo failed so that we can see
  // partial information even when GPU info collection fails. Handle malformed
  // angle::SystemInfo first.
  if (system_info->gpus.empty()) {
    return;
  }
  if (system_info->activeGPUIndex < 0) {
    system_info->activeGPUIndex = 0;
  }

  angle::GPUDeviceInfo* active =
      &system_info->gpus[system_info->activeGPUIndex];

  gpu_info->gpu.vendor_id = active->vendorId;
  gpu_info->gpu.device_id = active->deviceId;
#if BUILDFLAG(IS_CHROMEOS)
  gpu_info->gpu.revision = active->revisionId;
#endif  // BUILDFLAG(IS_CHROMEOS)
  gpu_info->gpu.system_device_id = active->systemDeviceId;
  gpu_info->gpu.driver_vendor = std::move(active->driverVendor);
  gpu_info->gpu.driver_version = std::move(active->driverVersion);
  gpu_info->gpu.active = true;

  for (size_t i = 0; i < system_info->gpus.size(); i++) {
    if (static_cast<int>(i) == system_info->activeGPUIndex) {
      continue;
    }

    GPUInfo::GPUDevice device;
    device.vendor_id = system_info->gpus[i].vendorId;
    device.device_id = system_info->gpus[i].deviceId;
#if BUILDFLAG(IS_CHROMEOS)
    device.revision = system_info->gpus[i].revisionId;
#endif  // BUILDFLAG(IS_CHROMEOS)
    device.system_device_id = system_info->gpus[i].systemDeviceId;
    device.driver_vendor = std::move(system_info->gpus[i].driverVendor);
    device.driver_version = std::move(system_info->gpus[i].driverVersion);

    gpu_info->secondary_gpus.push_back(device);
  }

  gpu_info->optimus = system_info->isOptimus;
  gpu_info->amd_switchable = system_info->isAMDSwitchable;

  gpu_info->machine_model_name = system_info->machineModelName;
  gpu_info->machine_model_version = system_info->machineModelVersion;
}

void CollectGraphicsInfoForTesting(GPUInfo* gpu_info) {
  DCHECK(gpu_info);
#if BUILDFLAG(IS_ANDROID)
  CollectContextGraphicsInfo(gpu_info);
#else
  CollectBasicGraphicsInfo(gpu_info);
#endif  // BUILDFLAG(IS_ANDROID)
}

bool CollectGpuExtraInfo(gfx::GpuExtraInfo* gpu_extra_info,
                         const GpuPreferences& prefs) {
  TRACE_EVENT("gpu,startup", "gpu_info_collector::CollectGpuExtraInfo");
  // Populate the list of ANGLE features by querying the functions exposed by
  // EGL_ANGLE_feature_control if it's available.
  if (gl::g_driver_egl.client_ext.b_EGL_ANGLE_feature_control) {
    EGLDisplay display = gl::GLSurfaceEGL::GetGLDisplayEGL()->GetDisplay();
    EGLAttrib feature_count = 0;
    eglQueryDisplayAttribANGLE(display, EGL_FEATURE_COUNT_ANGLE,
                               &feature_count);
    gpu_extra_info->angle_features.resize(static_cast<size_t>(feature_count));
    for (size_t i = 0; i < gpu_extra_info->angle_features.size(); i++) {
      gpu_extra_info->angle_features[i].name =
          QueryEGLStringi(display, EGL_FEATURE_NAME_ANGLE, i);
      gpu_extra_info->angle_features[i].category =
          QueryEGLStringi(display, EGL_FEATURE_CATEGORY_ANGLE, i);
      gpu_extra_info->angle_features[i].status =
          QueryEGLStringi(display, EGL_FEATURE_STATUS_ANGLE, i);
    }
  }

  return true;
}

// TODO(crbug.com/351564777): should be UNSAFE_BUFFER_USAGE
void CollectDawnInfo(const gpu::GpuPreferences& gpu_preferences,
                     bool collect_metrics,
                     std::vector<std::string>* dawn_info_list) {
  TRACE_EVENT("gpu,startup", "gpu_info_collector::CollectDawnInfo");
}

}  // namespace gpu
