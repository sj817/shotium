// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/utility/browser_exposed_utility_interfaces.h"

#include <stdint.h>

#include <utility>

#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/task/single_thread_task_runner.h"
#include "build/build_config.h"
#include "content/public/common/content_client.h"
#include "content/public/utility/content_utility_client.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "sandbox/policy/mojom/sandbox.mojom.h"
#include "sandbox/policy/sandbox_type.h"

#if !BUILDFLAG(IS_ANDROID)
#include "content/public/common/resource_usage_reporter.mojom.h"
#endif

namespace content {

namespace {

#if !BUILDFLAG(IS_ANDROID)
class ResourceUsageReporterImpl : public mojom::ResourceUsageReporter {
 public:
  ResourceUsageReporterImpl() = default;
  ResourceUsageReporterImpl(const ResourceUsageReporterImpl&) = delete;
  ~ResourceUsageReporterImpl() override = default;

  ResourceUsageReporterImpl& operator=(const ResourceUsageReporterImpl&) =
      delete;

 private:
  void GetUsageData(GetUsageDataCallback callback) override {
    mojom::ResourceUsageDataPtr data = mojom::ResourceUsageData::New();
    // The utility process's only V8 heap belonged to ProxyResolverV8, which
    // evaluated PAC scripts. It is deleted along with the rest of V8, so
    // reports_v8_stats stays false and the v8_bytes_* fields stay zero --
    // which is what "no V8 heap" means, rather than a heap of size zero.
    std::move(callback).Run(std::move(data));
  }
};

void CreateResourceUsageReporter(
    mojo::PendingReceiver<mojom::ResourceUsageReporter> receiver) {
  mojo::MakeSelfOwnedReceiver(std::make_unique<ResourceUsageReporterImpl>(),
                              std::move(receiver));
}
#endif  // !BUILDFLAG(IS_ANDROID)

}  // namespace

void ExposeUtilityInterfacesToBrowser(mojo::BinderMap* binders) {
#if !BUILDFLAG(IS_ANDROID)
  bool bind_usage_reporter = true;
#if BUILDFLAG(IS_WIN)
  auto& cmd_line = *base::CommandLine::ForCurrentProcess();
  if (sandbox::policy::SandboxTypeFromCommandLine(cmd_line) ==
      sandbox::mojom::Sandbox::kNoSandboxAndElevatedPrivileges) {
    bind_usage_reporter = false;
  }
#endif  // BUILDFLAG(IS_WIN)
  if (bind_usage_reporter) {
    binders->Add<mojom::ResourceUsageReporter>(
        &CreateResourceUsageReporter,
        base::SingleThreadTaskRunner::GetCurrentDefault());
  }
#endif  // !BUILDFLAG(IS_ANDROID)

  GetContentClient()->utility()->ExposeInterfacesToBrowser(binders);
}

}  // namespace content
