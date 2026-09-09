// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_ENGINE_H_
#define SHOT_SHOT_ENGINE_H_

#include <memory>
#include <string>

#include "base/functional/callback.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "shot/shot_cache.h"
#include "shot/shot_capture.h"
#include "shot/shot_network.h"

namespace shot {

// Internal, same-toolchain service. Neither Node values nor C ABI allocations
// cross this interface. Adapters own their transport and result conversion.
enum class EngineStatus { kOk, kUsage, kCapture, kState };
struct EngineOptions {
  NetworkConfig network;
  base::FilePath resource_dir;
  bool allow_file_access = false;
};
struct CacheOptions {
  base::FilePath directory;
  CacheClearOptions clear;
};
base::expected<EngineOptions, std::string> ReadEngineOptions(
    const base::DictValue& dict);
base::expected<CacheOptions, std::string> ReadCacheOptions(
    const base::DictValue& dict);

struct EngineResult {
  EngineStatus status = EngineStatus::kOk;
  std::string error;
  Bytes image;
  std::vector<DeliveredTile> tiles;
  std::optional<CaptureStats> stats;
  base::Value value;
};
using EngineCompletion = base::OnceCallback<void(EngineResult)>;

class EngineService {
 public:
  static base::expected<std::shared_ptr<EngineService>, std::string> Create(
      EngineOptions options);
  ~EngineService();
  EngineService(const EngineService&) = delete;
  EngineService& operator=(const EngineService&) = delete;

  // Accepted work completes once, on the engine thread. Rejected submissions
  // return false without invoking completion. Stop drains accepted work and
  // joins; callers must not call it from a completion or the engine thread.
  bool Capture(ScreenshotRequest request,
               bool tiles,
               EngineCompletion completion);
  bool Cache(CacheOptions options, bool clearing, EngineCompletion completion);
  EngineResult CaptureSync(ScreenshotRequest request, bool tiles);
  EngineResult CacheSync(CacheOptions options, bool clearing);
  base::DictValue Status();
  void Purge(bool release_working_set);
  void Stop();
  bool default_allow_file_access() const;

 private:
  EngineService();
  class Impl;
  std::unique_ptr<Impl> impl_;
};

// Before Blink exists, cache diagnostics need their own temporary environment.
// This synchronous entry is called on a dedicated native thread by Node.
EngineResult CacheWithoutEngine(CacheOptions options, bool clearing);

}  // namespace shot
#endif  // SHOT_SHOT_ENGINE_H_
