// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http/alternative_service.h"

#include "base/check_op.h"
#include "base/metrics/histogram_macros.h"
#include "base/metrics/histogram_macros_local.h"
#include "base/notreached.h"
#include "base/strings/stringprintf.h"
#include "net/base/port_util.h"

namespace net {

void HistogramAlternateProtocolUsage(AlternateProtocolUsage usage,
                                     bool is_google_host) {
  UMA_HISTOGRAM_ENUMERATION("Net.AlternateProtocolUsage", usage,
                            ALTERNATE_PROTOCOL_USAGE_MAX);
  if (is_google_host) {
    UMA_HISTOGRAM_ENUMERATION("Net.AlternateProtocolUsageGoogle", usage,
                              ALTERNATE_PROTOCOL_USAGE_MAX);
  }
}

void HistogramBrokenAlternateProtocolLocation(
    BrokenAlternateProtocolLocation location) {
  UMA_HISTOGRAM_ENUMERATION("Net.AlternateProtocolBrokenLocation", location,
                            BROKEN_ALTERNATE_PROTOCOL_LOCATION_MAX);
}

bool IsAlternateProtocolValid(NextProto protocol) {
  switch (protocol) {
    case NextProto::kProtoUnknown:
      return false;
    case NextProto::kProtoHTTP11:
      return false;
    case NextProto::kProtoHTTP2:
      return true;
    case NextProto::kProtoQUIC:
      // HTTP/3 is not built. NextProto keeps the constant because it is the
      // ALPN vocabulary, but nothing may advertise us onto it.
      return false;
  }
  NOTREACHED();
}

bool IsProtocolEnabled(NextProto protocol, bool is_http2_enabled) {
  switch (protocol) {
    case NextProto::kProtoUnknown:
      NOTREACHED();
    case NextProto::kProtoHTTP11:
      return true;
    case NextProto::kProtoHTTP2:
      return is_http2_enabled;
    case NextProto::kProtoQUIC:
      return false;
  }
  NOTREACHED();
}

AlternativeService::AlternativeService(NextProto protocol,
                                       std::string_view host,
                                       uint16_t port)
    : protocol(protocol), host(host), port(port) {}

AlternativeService::AlternativeService(NextProto protocol,
                                       const HostPortPair& host_port_pair)
    : AlternativeService(protocol,
                         host_port_pair.host(),
                         host_port_pair.port()) {}

AlternativeService::AlternativeService(
    const AlternativeService& alternative_service) = default;
AlternativeService::AlternativeService(AlternativeService&&) noexcept = default;

AlternativeService& AlternativeService::operator=(
    const AlternativeService& alternative_service) = default;
AlternativeService& AlternativeService::operator=(AlternativeService&&) =
    default;

HostPortPair AlternativeService::GetHostPortPair() const {
  return HostPortPair(host, port);
}

std::strong_ordering AlternativeService::operator<=>(
    const AlternativeService& other) const = default;

std::string AlternativeService::ToString() const {
  return base::StringPrintf("%s %s:%d", NextProtoToString(protocol),
                            host.c_str(), port);
}

std::ostream& operator<<(std::ostream& os,
                         const AlternativeService& alternative_service) {
  os << alternative_service.ToString();
  return os;
}

// static
AlternativeServiceInfo
AlternativeServiceInfo::CreateHttp2AlternativeServiceInfo(
    const AlternativeService& alternative_service,
    base::Time expiration) {
  DCHECK_EQ(alternative_service.protocol, NextProto::kProtoHTTP2);
  return AlternativeServiceInfo(alternative_service, expiration);
}

AlternativeServiceInfo::AlternativeServiceInfo() = default;

AlternativeServiceInfo::AlternativeServiceInfo(
    const AlternativeServiceInfo& alternative_service_info) = default;
AlternativeServiceInfo::AlternativeServiceInfo(
    AlternativeServiceInfo&&) noexcept = default;

AlternativeServiceInfo& AlternativeServiceInfo::operator=(
    AlternativeServiceInfo&&) = default;
AlternativeServiceInfo& AlternativeServiceInfo::operator=(
    const AlternativeServiceInfo& alternative_service_info) = default;

AlternativeServiceInfo::~AlternativeServiceInfo() = default;

bool AlternativeServiceInfo::operator==(
    const AlternativeServiceInfo& other) const = default;

std::string AlternativeServiceInfo::ToString() const {
  // NOTE: Cannot use `base::UnlocalizedTimeFormatWithPattern()` since
  // `net/DEPS` disallows `base/i18n`.
  base::Time::Exploded exploded;
  expiration_.LocalExplode(&exploded);
  return base::StringPrintf(
      "%s, expires %04d-%02d-%02d %02d:%02d:%02d",
      alternative_service_.ToString().c_str(), exploded.year, exploded.month,
      exploded.day_of_month, exploded.hour, exploded.minute, exploded.second);
}

AlternativeServiceInfoVector ProcessAlternativeServices(
    const spdy::SpdyAltSvcWireFormat::AlternativeServiceVector&
        alternative_service_vector,
    bool is_http2_enabled) {
  // Convert spdy::SpdyAltSvcWireFormat::AlternativeService entries
  // to AlternativeServiceInfo.
  //
  // Upstream this loop has a second arm: an entry whose protocol_id is not a
  // known NextProto is handed to quic::SpdyUtils::ExtractQuicVersionFromAltSvcEntry
  // to see whether it names an HTTP/3 version, and an entry that says "quic"
  // is dropped as a legacy advertisement. With HTTP/3 out of the build both
  // arms end in the same place, so what is left keeps only the "h2" entries --
  // IsAlternateProtocolValid() is false for everything else, kProtoQUIC
  // included.
  AlternativeServiceInfoVector alternative_service_info_vector;
  for (const spdy::SpdyAltSvcWireFormat::AlternativeService&
           alternative_service_entry : alternative_service_vector) {
    if (!IsPortValid(alternative_service_entry.port)) {
      continue;
    }

    NextProto protocol =
        NextProtoFromString(alternative_service_entry.protocol_id);
    if (!IsAlternateProtocolValid(protocol) ||
        !IsProtocolEnabled(protocol, is_http2_enabled)) {
      continue;
    }

    AlternativeService alternative_service(protocol,
                                           alternative_service_entry.host,
                                           alternative_service_entry.port);
    base::Time expiration =
        base::Time::Now() +
        base::Seconds(alternative_service_entry.max_age_seconds);
    alternative_service_info_vector.push_back(
        AlternativeServiceInfo::CreateHttp2AlternativeServiceInfo(
            alternative_service, expiration));
  }
  return alternative_service_info_vector;
}

AlternativeServiceInfo::AlternativeServiceInfo(
    const AlternativeService& alternative_service,
    base::Time expiration)
    : alternative_service_(alternative_service), expiration_(expiration) {}

}  // namespace net
