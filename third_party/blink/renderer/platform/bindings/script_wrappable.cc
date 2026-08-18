// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"

namespace blink {

const char* ScriptWrappable::GetHumanReadableName() const {
  return "ScriptWrappable";
}

void ScriptWrappable::Trace(Visitor*) const {}

}  // namespace blink
