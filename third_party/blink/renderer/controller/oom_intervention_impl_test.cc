// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/controller/oom_intervention_impl.h"

#include <unistd.h>

#include <utility>

#include "base/run_loop.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/public/common/oom_intervention/oom_intervention_types.h"
#include "third_party/blink/public/platform/scheduler/test/renderer_scheduler_test_support.h"
#include "third_party/blink/renderer/controller/crash_memory_metrics_reporter_impl.h"
#include "third_party/blink/renderer/core/exported/web_view_impl.h"
#include "third_party/blink/renderer/core/frame/frame_test_helpers.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/html_element.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/testing/task_environment.h"
#include "third_party/blink/renderer/platform/testing/unit_test_helpers.h"
#include "third_party/blink/renderer/platform/testing/url_test_helpers.h"

namespace blink {

namespace {

const uint64_t kTestPMFThreshold = 160 * 1024;

class MockOomInterventionHost : public mojom::blink::OomInterventionHost {
 public:
  MockOomInterventionHost(
      mojo::PendingReceiver<mojom::blink::OomInterventionHost> receiver)
      : receiver_(this, std::move(receiver)) {}
  ~MockOomInterventionHost() override = default;

  void OnHighMemoryUsage() override {}

 private:
  mojo::Receiver<mojom::blink::OomInterventionHost> receiver_;
};

// Mock that allows setting mock memory usage.
class MockMemoryUsageMonitor : public MemoryUsageMonitor {
 public:
  MockMemoryUsageMonitor() = default;

  MemoryUsage GetCurrentMemoryUsage() override { return mock_memory_usage_; }

  // MemoryUsageMonitor will report the current memory usage as this value.
  void SetMockMemoryUsage(MemoryUsage usage) { mock_memory_usage_ = usage; }

 private:
  MemoryUsage mock_memory_usage_;
};

// Mock intervention class that uses a mock MemoryUsageMonitor.
class MockOomInterventionImpl : public OomInterventionImpl {
 public:
  MockOomInterventionImpl()
      : OomInterventionImpl(scheduler::GetSingleThreadTaskRunnerForTesting()),
        mock_memory_usage_monitor_(std::make_unique<MockMemoryUsageMonitor>()) {
  }
  ~MockOomInterventionImpl() override {}

  MemoryUsageMonitor& MemoryUsageMonitorInstance() override {
    return *mock_memory_usage_monitor_;
  }

  MockMemoryUsageMonitor* mock_memory_usage_monitor() {
    return mock_memory_usage_monitor_.get();
  }

 private:
  std::unique_ptr<OomInterventionMetrics> metrics_;
  std::unique_ptr<MockMemoryUsageMonitor> mock_memory_usage_monitor_;
};

}  // namespace

class OomInterventionImplTest : public testing::Test {
 public:
  void SetUp() override {
    intervention_ = std::make_unique<MockOomInterventionImpl>();
  }

  Page* DetectOnceOnBlankPage() {
    WebViewImpl* web_view = web_view_helper_.InitializeAndLoad("about:blank");
    Page* page = web_view->MainFrameImpl()->GetFrame()->GetPage();
    EXPECT_FALSE(page->Paused());
    RunDetection(true, false);
    return page;
  }

  void RunDetection(bool renderer_pause_enabled,
                    bool purge_v8_memory_enabled) {
    mojo::PendingRemote<mojom::blink::OomInterventionHost> remote_host;
    MockOomInterventionHost mock_host(
        remote_host.InitWithNewPipeAndPassReceiver());

    mojom::blink::DetectionArgsPtr args(mojom::blink::DetectionArgs::New());
    args->private_footprint_threshold = kTestPMFThreshold;

    intervention_->StartDetection(std::move(remote_host), std::move(args),
                                  renderer_pause_enabled,
                                  purge_v8_memory_enabled);
    test::RunDelayedTasks(base::Seconds(1));
  }

 protected:
  test::TaskEnvironment task_environment_;
  std::unique_ptr<MockOomInterventionImpl> intervention_;
  frame_test_helpers::WebViewHelper web_view_helper_;
  std::unique_ptr<SimRequest> main_resource_;
};

TEST_F(OomInterventionImplTest, NoDetectionOnBelowThreshold) {
  MemoryUsage usage;
  // Set value less than the threshold to not trigger intervention.
  usage.private_footprint_bytes = kTestPMFThreshold - 1024;
  usage.swap_bytes = 0;
  usage.vm_size_bytes = 0;
  intervention_->mock_memory_usage_monitor()->SetMockMemoryUsage(usage);

  Page* page = DetectOnceOnBlankPage();

  EXPECT_FALSE(page->Paused());
}

TEST_F(OomInterventionImplTest, PmfThresholdDetection) {
  MemoryUsage usage;
  // Set value more than the threshold to trigger intervention.
  usage.private_footprint_bytes = kTestPMFThreshold + 1024;
  usage.swap_bytes = 0;
  usage.vm_size_bytes = 0;
  intervention_->mock_memory_usage_monitor()->SetMockMemoryUsage(usage);

  Page* page = DetectOnceOnBlankPage();

  EXPECT_TRUE(page->Paused());
  intervention_.reset();
  EXPECT_FALSE(page->Paused());
}

TEST_F(OomInterventionImplTest, StopWatchingAfterDetection) {
  MemoryUsage usage;
  // Set value more than the threshold to trigger intervention.
  usage.private_footprint_bytes = kTestPMFThreshold + 1024;
  usage.swap_bytes = 0;
  usage.vm_size_bytes = 0;
  intervention_->mock_memory_usage_monitor()->SetMockMemoryUsage(usage);

  DetectOnceOnBlankPage();

  EXPECT_FALSE(intervention_->mock_memory_usage_monitor()->HasObserver(
      intervention_.get()));
}

TEST_F(OomInterventionImplTest, ContinueWatchingWithoutDetection) {
  MemoryUsage usage;
  // Set value less than the threshold to not trigger intervention.
  usage.private_footprint_bytes = 0;
  usage.swap_bytes = 0;
  usage.vm_size_bytes = 0;
  intervention_->mock_memory_usage_monitor()->SetMockMemoryUsage(usage);

  DetectOnceOnBlankPage();

  EXPECT_TRUE(intervention_->mock_memory_usage_monitor()->HasObserver(
      intervention_.get()));
}

// V1DetectionAdsNavigation was here. It tagged an iframe with a FrameAdEvidence
// and expected the intervention to navigate it to about:blank. Ad tagging is
// gone, and with it OomIntervention's navigate-ads leg.

TEST_F(OomInterventionImplTest, V2DetectionV8PurgeMemory) {
  MemoryUsage usage;
  // Set value more than the threshold to trigger intervention.
  usage.private_footprint_bytes = kTestPMFThreshold + 1024;
  usage.swap_bytes = 0;
  usage.vm_size_bytes = 0;
  intervention_->mock_memory_usage_monitor()->SetMockMemoryUsage(usage);

  WebViewImpl* web_view = web_view_helper_.InitializeAndLoad("about:blank");
  Page* page = web_view->MainFrameImpl()->GetFrame()->GetPage();
  auto* frame = To<LocalFrame>(page->MainFrame());
  EXPECT_FALSE(frame->DomWindow()->IsContextDestroyed());
  RunDetection(true, true);
  EXPECT_TRUE(frame->DomWindow()->IsContextDestroyed());
}

}  // namespace blink
