// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/render_process_impl.h"

#include "build/build_config.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>

#include <mlang.h>
#include <objidl.h>
#endif

#include <stddef.h>

#include <algorithm>
#include <utility>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/debug/crash_logging.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/synchronization/waitable_event.h"
#include "base/system/sys_info.h"
#include "base/task/task_features.h"
#include "base/task/thread_pool/initialization_util.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "content/common/features.h"
#include "content/common/thread_pool_util.h"
#include "content/public/common/bindings_policy.h"
#include "content/public/common/content_client.h"
#include "content/public/common/content_features.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/content_renderer_client.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/common/features.h"
#include "third_party/blink/public/platform/web_runtime_features.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_frame.h"

#if BUILDFLAG(IS_WIN)
#include "base/win/win_util.h"
#endif

namespace {

std::unique_ptr<base::ThreadPoolInstance::InitParams>
GetThreadPoolInitParams() {
  constexpr size_t kMaxNumThreadsInForegroundPoolLowerBound = 3;
  size_t desired_num_threads =
      std::max(kMaxNumThreadsInForegroundPoolLowerBound,
               content::GetMinForegroundThreadsInRendererThreadPool());
  return std::make_unique<base::ThreadPoolInstance::InitParams>(
      desired_num_threads);
}

}  // namespace

namespace content {

RenderProcessImpl::RenderProcessImpl()
    : RenderProcess(GetThreadPoolInitParams()) {
  // This build has no JavaScript engine, so every V8 flag and WebAssembly
  // trap-handler switch that used to be applied here has no effect and is
  // gone. The SharedArrayBuffer *Blink* runtime feature below is a separate
  // thing: it gates the IDL-visible constructor, not a V8 flag, so the
  // Enterprise-policy bypass is still honoured.
#if !BUILDFLAG(IS_ANDROID)
  if (!base::FeatureList::IsEnabled(features::kSharedArrayBuffer) &&
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kSharedArrayBufferUnrestrictedAccessAllowed)) {
    blink::WebRuntimeFeatures::EnableSharedArrayBufferUnrestrictedAccessAllowed(
        true);
  }
#endif
}

RenderProcessImpl::~RenderProcessImpl() {
#ifndef NDEBUG
  int count = blink::WebFrame::InstanceCount();
  if (count)
    DLOG(ERROR) << "WebFrame LEAKED " << count << " TIMES";
#endif

  GetShutDownEvent()->Signal();
}

std::unique_ptr<RenderProcess> RenderProcessImpl::Create() {
  return base::WrapUnique(new RenderProcessImpl());
}

void RenderProcessImpl::AddRefProcess() {
  NOTREACHED();
}

void RenderProcessImpl::ReleaseProcess() {
  NOTREACHED();
}

}  // namespace content
