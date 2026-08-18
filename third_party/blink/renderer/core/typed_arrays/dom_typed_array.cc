// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/typed_arrays/dom_typed_array.h"

namespace blink {

#define DOMTYPEDARRAY_EXPLICITLY_INSTANTIATE(val_t, Type, clamped) \
  template class CORE_TEMPLATE_EXPORT                              \
      DOMTypedArray<val_t, Type##ArrayTag, clamped>;
DOMTYPEDARRAY_FOREACH_VIEW_TYPE(DOMTYPEDARRAY_EXPLICITLY_INSTANTIATE)
#undef DOMTYPEDARRAY_EXPLICITLY_INSTANTIATE

}  // namespace blink
