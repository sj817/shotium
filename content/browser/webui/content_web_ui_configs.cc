// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/webui/content_web_ui_configs.h"

#include "content/public/browser/webui_config_map.h"

namespace content {

// Every chrome://*-internals page Content used to register here has been
// deleted: gpu, indexed-db, media, histograms, network-errors, process,
// quota, service-worker, ukm, webrtc, webxr and the two tracing pages. They
// were debugging surfaces for a browser product, and each one anchored a
// TypeScript resource tree under ui/webui/resources plus the subsystem it
// reported on.
//
// The function itself is kept so the single call site in
// BrowserMainLoop::MainMessageLoopStart does not need a build-flag branch.
void RegisterContentWebUIConfigs() {}

}  // namespace content
