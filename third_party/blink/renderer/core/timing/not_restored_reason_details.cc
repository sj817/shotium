// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/timing/not_restored_reason_details.h"

#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {

NotRestoredReasonDetails::NotRestoredReasonDetails(String reason)
    : reason_(reason) {}

NotRestoredReasonDetails::NotRestoredReasonDetails(
    const NotRestoredReasonDetails& other)
    : reason_(other.reason_) {}

}  // namespace blink
