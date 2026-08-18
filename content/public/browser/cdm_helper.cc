// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/browser/cdm_helper.h"

#include <memory>


namespace content {

CdmHelper::CdmHelper() = default;

CdmHelper::~CdmHelper() = default;

// static
std::unique_ptr<CdmHelper> CdmHelper::CreateInstance() {
  return std::make_unique<CdmHelperImpl>();
}

}  // namespace content
