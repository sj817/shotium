// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_EVENTS_MOJOM_EVENT_ENUM_MOJOM_TRAITS_H_
#define UI_EVENTS_MOJOM_EVENT_ENUM_MOJOM_TRAITS_H_

#include "base/check_op.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "ui/events/event_constants.h"
#include "ui/events/mojom/event_constants.mojom-shared.h"
#include "ui/events/mojom/scroll_granularity.mojom-shared.h"
#include "ui/events/types/scroll_types.h"

namespace mojo {

template <>
struct EnumTraits<ui::mojom::EventPointerType, ui::EventPointerType> {
  static ui::mojom::EventPointerType ToMojom(ui::EventPointerType input) {
    CHECK_GE(input, ui::EventPointerType::kUnknown);
    CHECK_LE(input, ui::EventPointerType::kMaxValue);
    return static_cast<ui::mojom::EventPointerType>(input);
  }
  static bool FromMojom(ui::mojom::EventPointerType input,
                       ui::EventPointerType* output) {
    const int value = static_cast<int>(input);
    if (value < 0 || value > static_cast<int>(ui::EventPointerType::kMaxValue)) {
      return false;
    }
    *output = static_cast<ui::EventPointerType>(value);
    return true;
  }
};

template <>
struct EnumTraits<ui::mojom::ScrollGranularity, ui::ScrollGranularity> {
  static ui::mojom::ScrollGranularity ToMojom(ui::ScrollGranularity input) {
    CHECK_GE(input, ui::ScrollGranularity::kFirstScrollGranularity);
    CHECK_LE(input, ui::ScrollGranularity::kMaxValue);
    return static_cast<ui::mojom::ScrollGranularity>(input);
  }
  static bool FromMojom(ui::mojom::ScrollGranularity input,
                       ui::ScrollGranularity* output) {
    const int value = static_cast<int>(input);
    if (value < static_cast<int>(ui::ScrollGranularity::kFirstScrollGranularity) ||
        value > static_cast<int>(ui::ScrollGranularity::kMaxValue)) {
      return false;
    }
    *output = static_cast<ui::ScrollGranularity>(value);
    return true;
  }
};

}  // namespace mojo

#endif  // UI_EVENTS_MOJOM_EVENT_ENUM_MOJOM_TRAITS_H_
