// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SERVICES_NETWORK_PUBLIC_CPP_NETWORK_TYPES_MOJOM_TRAITS_H_
#define SERVICES_NETWORK_PUBLIC_CPP_NETWORK_TYPES_MOJOM_TRAITS_H_

#include "base/check_op.h"
#include "mojo/public/cpp/bindings/enum_traits.h"
#include "net/http/http_connection_info.h"
#include "net/nqe/effective_connection_type.h"
#include "services/network/public/mojom/network_types.mojom-shared.h"

namespace mojo {

template <>
struct EnumTraits<network::mojom::ConnectionInfo, net::HttpConnectionInfo> {
  static network::mojom::ConnectionInfo ToMojom(net::HttpConnectionInfo input) {
    const int value = static_cast<int>(input);
    CHECK_GE(value, 0);
    CHECK_LE(value, static_cast<int>(net::HttpConnectionInfo::kMaxValue));
    return static_cast<network::mojom::ConnectionInfo>(value);
  }

  static bool FromMojom(network::mojom::ConnectionInfo input,
                       net::HttpConnectionInfo* output) {
    const int value = static_cast<int>(input);
    if (value < 0 ||
        value > static_cast<int>(net::HttpConnectionInfo::kMaxValue)) {
      return false;
    }
    *output = static_cast<net::HttpConnectionInfo>(value);
    return true;
  }
};

template <>
struct EnumTraits<network::mojom::EffectiveConnectionType,
                  net::EffectiveConnectionType> {
  static network::mojom::EffectiveConnectionType ToMojom(
      net::EffectiveConnectionType input) {
    CHECK_GE(input, net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN);
    CHECK_LT(input, net::EFFECTIVE_CONNECTION_TYPE_LAST);
    return static_cast<network::mojom::EffectiveConnectionType>(input);
  }

  static bool FromMojom(network::mojom::EffectiveConnectionType input,
                       net::EffectiveConnectionType* output) {
    const int value = static_cast<int>(input);
    if (value < net::EFFECTIVE_CONNECTION_TYPE_UNKNOWN ||
        value >= net::EFFECTIVE_CONNECTION_TYPE_LAST) {
      return false;
    }
    *output = static_cast<net::EffectiveConnectionType>(value);
    return true;
  }
};

static_assert(static_cast<int>(network::mojom::ConnectionInfo::kMaxValue) ==
              static_cast<int>(net::HttpConnectionInfo::kMaxValue));
static_assert(static_cast<int>(network::mojom::EffectiveConnectionType::kMaxValue) ==
              net::EFFECTIVE_CONNECTION_TYPE_LAST - 1);

}  // namespace mojo

#endif  // SERVICES_NETWORK_PUBLIC_CPP_NETWORK_TYPES_MOJOM_TRAITS_H_
