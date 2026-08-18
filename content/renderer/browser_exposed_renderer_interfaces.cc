// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/browser_exposed_renderer_interfaces.h"

#include <stdint.h>

#include <memory>
#include <utility>

#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/task_runner.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "content/common/frame.mojom.h"
#include "content/public/common/content_client.h"
#include "content/public/common/resource_usage_reporter.mojom.h"
#include "content/public/common/resource_usage_reporter_type_converters.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/renderer/render_thread_impl.h"
#include "mojo/public/cpp/bindings/binder_map.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"


namespace content {

namespace {


class ResourceUsageReporterImpl : public content::mojom::ResourceUsageReporter {
 public:
  explicit ResourceUsageReporterImpl(base::WeakPtr<RenderThread> thread)
      : thread_(std::move(thread)) {}
  ResourceUsageReporterImpl(const ResourceUsageReporterImpl&) = delete;
  ~ResourceUsageReporterImpl() override = default;

  ResourceUsageReporterImpl& operator=(const ResourceUsageReporterImpl&) =
      delete;

 private:
  void SendResults() {
    if (!callback_.is_null())
      std::move(callback_).Run(std::move(usage_data_));
    callback_.Reset();
    weak_factory_.InvalidateWeakPtrs();
  }

  void GetUsageData(GetUsageDataCallback callback) override {
    DCHECK(callback_.is_null());
    weak_factory_.InvalidateWeakPtrs();
    usage_data_ = mojom::ResourceUsageData::New();
    // There is no JavaScript engine in this build, so there are no V8 heap
    // numbers to report. Leave `v8_bytes_*` at zero and tell the browser not
    // to interpret them.
    usage_data_->reports_v8_stats = false;
    callback_ = std::move(callback);

    // Since it is not safe to call any Blink functions until Blink has been
    // initialized, early out and send 0 back for all resources.
    if (!thread_) {
      SendResults();
      return;
    }

    blink::WebCacheResourceTypeStats stats;
    blink::WebCache::GetResourceTypeStats(&stats);
    usage_data_->web_cache_stats = mojom::ResourceTypeStats::From(stats);

    SendResults();
  }

  const base::WeakPtr<RenderThread> thread_;
  mojom::ResourceUsageDataPtr usage_data_;
  GetUsageDataCallback callback_;

  base::WeakPtrFactory<ResourceUsageReporterImpl> weak_factory_{this};
};

void CreateResourceUsageReporter(
    base::WeakPtr<RenderThreadImpl> render_thread,
    mojo::PendingReceiver<mojom::ResourceUsageReporter> receiver) {
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<ResourceUsageReporterImpl>(std::move(render_thread)),
      std::move(receiver));
}

}  // namespace

void ExposeRendererInterfacesToBrowser(
    base::WeakPtr<RenderThreadImpl> render_thread,
    mojo::BinderMap* binders) {
  DCHECK(render_thread);

  // No blink::mojom::SharedWorkerFactory or
  // blink::mojom::EmbeddedWorkerInstanceClient binder is registered: this build
  // has no script engine, so neither shared workers nor service workers can
  // run. The browser sees the interface requests fail rather than getting a
  // client that would accept a start request and never execute it.
  binders->Add<mojom::ResourceUsageReporter>(
      base::BindRepeating(&CreateResourceUsageReporter, render_thread),
      base::SingleThreadTaskRunner::GetCurrentDefault());

  GetContentClient()->renderer()->ExposeInterfacesToBrowser(binders);
}

}  // namespace content
