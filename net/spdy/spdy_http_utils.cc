// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.


#include "net/spdy/spdy_http_utils.h"

#include <string>
#include <string_view>
#include <vector>

#include "base/check_op.h"
#include "base/feature_list.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/string_view_util.h"
#include "base/types/expected.h"
#include "base/types/expected_macros.h"
#include "net/base/features.h"
#include "net/base/url_util.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_request_info.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_response_info.h"
#include "net/http/http_util.h"
#include "net/third_party/quiche/src/quiche/common/structured_headers.h"

namespace net {

const char* const kHttp2PriorityHeader = "priority";

namespace {

// RFC 9218 "Extensible Prioritization Scheme for HTTP". The scheme is shared
// between HTTP/2 and HTTP/3; quiche houses it under quiche/quic/core/
// (quic_stream_priority.h) only because HTTP/3 was its first user there, and
// that header reaches quic_types.h and web_transport.h, neither of which is in
// this build. What the `priority` request header needs is just the structured-
// headers dictionary below, so it is written out here against
// quiche/common/structured_headers.h directly.
//
// Kept byte-identical to quic::SerializePriorityFieldValue(): defaults are
// omitted from the dictionary, out-of-range urgencies are dropped rather than
// clamped, and a serialization failure yields the empty string (the caller
// then sends no header at all).
constexpr int kPriorityMinimumUrgency = 0;
constexpr int kPriorityMaximumUrgency = 7;
constexpr int kPriorityDefaultUrgency = 3;
constexpr bool kPriorityDefaultIncremental = false;
constexpr std::string_view kPriorityUrgencyKey = "u";
constexpr std::string_view kPriorityIncrementalKey = "i";

// Maps net's RequestPriority onto an RFC 9218 urgency. This is the body of
// what net/quic/quic_http_utils.cc called
// ConvertRequestPriorityToQuicPriority().
int RequestPriorityToUrgency(RequestPriority priority) {
  DCHECK_GE(priority, MINIMUM_PRIORITY);
  DCHECK_LE(priority, MAXIMUM_PRIORITY);
  return HIGHEST - priority;
}

std::string SerializePriorityFieldValue(int urgency, bool incremental) {
  quiche::structured_headers::Dictionary dictionary;

  if (urgency != kPriorityDefaultUrgency && urgency >= kPriorityMinimumUrgency &&
      urgency <= kPriorityMaximumUrgency) {
    dictionary[std::string(kPriorityUrgencyKey)] =
        quiche::structured_headers::ParameterizedMember(
            quiche::structured_headers::Item(static_cast<int64_t>(urgency)),
            {});
  }

  if (incremental != kPriorityDefaultIncremental) {
    dictionary[std::string(kPriorityIncrementalKey)] =
        quiche::structured_headers::ParameterizedMember(
            quiche::structured_headers::Item(incremental), {});
  }

  return quiche::structured_headers::SerializeDictionary(dictionary)
      .value_or(std::string());
}

}  // namespace

namespace {

// The number of bytes to reserve for the raw headers string to avoid having to
// do reallocations most of the time. Equal to the 99th percentile of header
// sizes in ricea@'s cache on 3 Aug 2023.
constexpr size_t kExpectedRawHeaderSize = 4035;

// Add header `name` with `value` to `headers`. `name` must not already exist in
// `headers`.
void AddUniqueSpdyHeader(std::string_view name,
                         std::string_view value,
                         quiche::HttpHeaderBlock* headers) {
  auto insert_result = headers->insert({name, value});
  CHECK_EQ(insert_result, quiche::HttpHeaderBlock::InsertResult::kInserted);
}

// Convert `headers` to an HttpResponseHeaders object based on the features
// enabled at runtime.
base::expected<scoped_refptr<HttpResponseHeaders>, int>
SpdyHeadersToHttpResponseHeadersUsingFeatures(
    const quiche::HttpHeaderBlock& headers) {
  if (base::FeatureList::IsEnabled(
          features::kSpdyHeadersToHttpResponseUseBuilder)) {
    return SpdyHeadersToHttpResponseHeadersUsingBuilder(headers);
  } else {
    return SpdyHeadersToHttpResponseHeadersUsingRawString(headers);
  }
}

}  // namespace

int SpdyHeadersToHttpResponse(const quiche::HttpHeaderBlock& headers,
                              HttpResponseInfo* response) {
  ASSIGN_OR_RETURN(response->headers,
                   SpdyHeadersToHttpResponseHeadersUsingFeatures(headers));
  response->was_fetched_via_spdy = true;
  return OK;
}

NET_EXPORT_PRIVATE base::expected<scoped_refptr<HttpResponseHeaders>, int>
SpdyHeadersToHttpResponseHeadersUsingRawString(
    const quiche::HttpHeaderBlock& headers) {
  // The ":status" header is required.
  quiche::HttpHeaderBlock::const_iterator it =
      headers.find(spdy::kHttp2StatusHeader);
  if (it == headers.end()) {
    return base::unexpected(ERR_INCOMPLETE_HTTP2_HEADERS);
  }

  const auto status = it->second;

  std::string raw_headers = base::StrCat(
      {"HTTP/1.1 ", status, base::MakeStringViewWithNulChars("\0")});
  raw_headers.reserve(kExpectedRawHeaderSize);
  for (const auto& [name, value] : headers) {
    DCHECK_GT(name.size(), 0u);
    if (name[0] == ':') {
      // https://tools.ietf.org/html/rfc7540#section-8.1.2.4
      // Skip pseudo headers.
      continue;
    }
    // For each value, if the server sends a NUL-separated
    // list of values, we separate that back out into
    // individual headers for each value in the list.
    // e.g.
    //    Set-Cookie "foo\0bar"
    // becomes
    //    Set-Cookie: foo\0
    //    Set-Cookie: bar\0
    size_t start = 0;
    size_t end = 0;
    do {
      end = value.find('\0', start);
      std::string_view tval;
      if (end != value.npos) {
        tval = value.substr(start, (end - start));
      } else {
        tval = value.substr(start);
      }
      base::StrAppend(&raw_headers, {name, ":", tval,
                                     base::MakeStringViewWithNulChars("\0")});
      start = end + 1;
    } while (end != value.npos);
  }

  auto response_headers =
      base::MakeRefCounted<HttpResponseHeaders>(raw_headers);

  // When there are multiple location headers the response is a potential
  // response smuggling attack.
  if (HttpUtil::HeadersContainMultipleCopiesOfField(*response_headers,
                                                    "location")) {
    return base::unexpected(ERR_RESPONSE_HEADERS_MULTIPLE_LOCATION);
  }
  if (HttpUtil::HeadersContainMultipleCopiesOfField(*response_headers,
                                                    "content-disposition")) {
    return base::unexpected(ERR_RESPONSE_HEADERS_MULTIPLE_CONTENT_DISPOSITION);
  }

  return response_headers;
}

NET_EXPORT_PRIVATE base::expected<scoped_refptr<HttpResponseHeaders>, int>
SpdyHeadersToHttpResponseHeadersUsingBuilder(
    const quiche::HttpHeaderBlock& headers) {
  // The ":status" header is required.
  // TODO(ricea): The ":status" header should always come first. Skip this hash
  // lookup after we no longer need to be compatible with the old
  // implementation.
  quiche::HttpHeaderBlock::const_iterator it =
      headers.find(spdy::kHttp2StatusHeader);
  if (it == headers.end()) {
    return base::unexpected(ERR_INCOMPLETE_HTTP2_HEADERS);
  }

  const auto status = it->second;

  HttpResponseHeaders::Builder builder({1, 1}, status);

  for (const auto& [name, value] : headers) {
    DCHECK_GT(name.size(), 0u);
    if (name[0] == ':') {
      // https://tools.ietf.org/html/rfc7540#section-8.1.2.4
      // Skip pseudo headers.
      continue;
    }
    // For each value, if the server sends a NUL-separated
    // list of values, we separate that back out into
    // individual headers for each value in the list.
    // e.g.
    //    Set-Cookie "foo\0bar"
    // becomes
    //    Set-Cookie: foo\0
    //    Set-Cookie: bar\0
    size_t start = 0;
    size_t end = 0;
    do {
      end = value.find('\0', start);
      std::string_view tval;
      if (end != value.npos) {
        tval = value.substr(start, (end - start));
      } else {
        tval = value.substr(start);
      }
      builder.AddHeader(name, tval);
      start = end + 1;
    } while (end != value.npos);
  }

  auto response_headers = builder.Build();

  // When there are multiple location headers the response is a potential
  // response smuggling attack.
  if (HttpUtil::HeadersContainMultipleCopiesOfField(*response_headers,
                                                    "location")) {
    return base::unexpected(ERR_RESPONSE_HEADERS_MULTIPLE_LOCATION);
  }
  if (HttpUtil::HeadersContainMultipleCopiesOfField(*response_headers,
                                                    "content-disposition")) {
    return base::unexpected(ERR_RESPONSE_HEADERS_MULTIPLE_CONTENT_DISPOSITION);
  }

  return response_headers;
}

void CreateSpdyHeadersFromHttpRequest(const HttpRequestInfo& info,
                                      std::optional<RequestPriority> priority,
                                      const HttpRequestHeaders& request_headers,
                                      quiche::HttpHeaderBlock* headers) {
  headers->insert({spdy::kHttp2MethodHeader, info.method});
  if (info.method == "CONNECT") {
    headers->insert({spdy::kHttp2AuthorityHeader, GetHostAndPort(info.url)});
  } else {
    headers->insert(
        {spdy::kHttp2AuthorityHeader, GetHostAndOptionalPort(info.url)});
    headers->insert({spdy::kHttp2SchemeHeader, info.url.GetScheme()});
    headers->insert({spdy::kHttp2PathHeader, info.url.PathForRequest()});
  }

  HttpRequestHeaders::Iterator it(request_headers);
  while (it.GetNext()) {
    std::string name = base::ToLowerASCII(it.name());
    if (name.empty() || name[0] == ':' || name == "connection" ||
        name == "proxy-connection" || name == "transfer-encoding" ||
        name == "host") {
      continue;
    }
    AddUniqueSpdyHeader(name, it.value(), headers);
  }

  // Add the priority header if there is not already one set.
  if (priority &&
      headers->find(kHttp2PriorityHeader) == headers->end()) {
    std::string serialized_priority = SerializePriorityFieldValue(
        RequestPriorityToUrgency(priority.value()), info.priority_incremental);
    if (!serialized_priority.empty()) {
      AddUniqueSpdyHeader(kHttp2PriorityHeader, serialized_priority, headers);
    }
  }
}

void CreateSpdyHeadersFromHttpRequestForExtendedConnect(
    const HttpRequestInfo& info,
    std::optional<RequestPriority> priority,
    const std::string& ext_connect_protocol,
    const HttpRequestHeaders& request_headers,
    quiche::HttpHeaderBlock* headers) {
  CHECK_EQ(info.method, "CONNECT");

  // Extended CONNECT, unlike CONNECT, requires scheme and path, and uses the
  // default port in the authority header.
  headers->insert({spdy::kHttp2SchemeHeader, info.url.GetScheme()});
  headers->insert({spdy::kHttp2PathHeader, info.url.PathForRequest()});
  headers->insert({spdy::kHttp2ProtocolHeader, ext_connect_protocol});

  CreateSpdyHeadersFromHttpRequest(info, priority, request_headers, headers);

  // Replace the existing `:authority` header. This will still be ordered
  // correctly, since the header was first added before any regular headers.
  headers->insert(
      {spdy::kHttp2AuthorityHeader, GetHostAndOptionalPort(info.url)});
}

void CreateSpdyHeadersFromHttpRequestForWebSocket(
    const GURL& url,
    const HttpRequestHeaders& request_headers,
    quiche::HttpHeaderBlock* headers) {
  headers->insert({spdy::kHttp2MethodHeader, "CONNECT"});
  headers->insert({spdy::kHttp2AuthorityHeader, GetHostAndOptionalPort(url)});
  headers->insert({spdy::kHttp2SchemeHeader, "https"});
  headers->insert({spdy::kHttp2PathHeader, url.PathForRequest()});
  headers->insert({spdy::kHttp2ProtocolHeader, "websocket"});

  HttpRequestHeaders::Iterator it(request_headers);
  while (it.GetNext()) {
    std::string name = base::ToLowerASCII(it.name());
    if (name.empty() || name[0] == ':' || name == "upgrade" ||
        name == "connection" || name == "proxy-connection" ||
        name == "transfer-encoding" || name == "host") {
      continue;
    }
    AddUniqueSpdyHeader(name, it.value(), headers);
  }
}

static_assert(HIGHEST - LOWEST < 4 && HIGHEST - MINIMUM_PRIORITY < 6,
              "request priority incompatible with spdy");

spdy::SpdyPriority ConvertRequestPriorityToSpdyPriority(
    const RequestPriority priority) {
  DCHECK_GE(priority, MINIMUM_PRIORITY);
  DCHECK_LE(priority, MAXIMUM_PRIORITY);
  return static_cast<spdy::SpdyPriority>(MAXIMUM_PRIORITY - priority +
                                         spdy::kV3HighestPriority);
}

NET_EXPORT_PRIVATE RequestPriority
ConvertSpdyPriorityToRequestPriority(spdy::SpdyPriority priority) {
  // Handle invalid values gracefully.
  return ((priority - spdy::kV3HighestPriority) >
          (MAXIMUM_PRIORITY - MINIMUM_PRIORITY))
             ? IDLE
             : static_cast<RequestPriority>(
                   MAXIMUM_PRIORITY - (priority - spdy::kV3HighestPriority));
}

}  // namespace net
