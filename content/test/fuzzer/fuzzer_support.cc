// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/test/fuzzer/fuzzer_support.h"

#include <string>

#include "base/feature_list.h"
#include "base/i18n/icu_util.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/test/test_timeouts.h"
#include "third_party/blink/public/platform/web_runtime_features.h"

namespace content {

RenderViewTestAdapter::RenderViewTestAdapter()
    // Allow fuzzing test a longer Run() timeout than normal (see
    // htps://crbug.com/1053401).
    : increased_timeout_(FROM_HERE, TestTimeouts::action_max_timeout()) {}

void RenderViewTestAdapter::SetUp() {
  RenderViewTest::SetUp();
  CreateFakeURLLoaderFactory();
}

Env::Env() {
  base::CommandLine::Init(0, nullptr);
  base::FeatureList::InitInstance(std::string(), std::string());
  base::i18n::InitializeICU();
  TestTimeouts::Initialize();

  blink::WebRuntimeFeatures::EnableExperimentalFeatures(true);
  blink::WebRuntimeFeatures::EnableTestOnlyFeatures(true);

  adapter = std::make_unique<RenderViewTestAdapter>();
  adapter->SetUp();

  gin::IsolateHolder::Initialize(gin::IsolateHolder::kStrictMode,
                                 gin::ArrayBufferAllocator::SharedInstance());
}

Env::~Env() {
  LOG(FATAL) << "NOT SUPPORTED";
}

}  // namespace content
