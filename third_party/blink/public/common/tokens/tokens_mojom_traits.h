// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_COMMON_TOKENS_TOKENS_MOJOM_TRAITS_H_
#define THIRD_PARTY_BLINK_PUBLIC_COMMON_TOKENS_TOKENS_MOJOM_TRAITS_H_

#include "base/immediate_crash.h"
#include "third_party/blink/public/common/common_export.h"
#include "third_party/blink/public/common/tokens/token_mojom_traits_helper.h"
#include "third_party/blink/public/common/tokens/tokens.h"
#include "third_party/blink/public/mojom/tokens/tokens.mojom-shared.h"

namespace mojo {

// Mojom traits for the various token types.
// See third_party/blink/public/common/tokens/tokens.h for more details.

////////////////////////////////////////////////////////////////////////////////
// DOCUMENT TOKENS
template <>
struct StructTraits<blink::mojom::DocumentTokenDataView, blink::DocumentToken>
    : public blink::TokenMojomTraitsHelper<blink::mojom::DocumentTokenDataView,
                                           blink::DocumentToken> {};

////////////////////////////////////////////////////////////////////////////////
// FRAME TOKENS

template <>
struct StructTraits<blink::mojom::LocalFrameTokenDataView,
                    blink::LocalFrameToken>
    : public blink::TokenMojomTraitsHelper<
          blink::mojom::LocalFrameTokenDataView,
          blink::LocalFrameToken> {};

template <>
struct StructTraits<blink::mojom::RemoteFrameTokenDataView,
                    blink::RemoteFrameToken>
    : public blink::TokenMojomTraitsHelper<
          blink::mojom::RemoteFrameTokenDataView,
          blink::RemoteFrameToken> {};

template <>
struct BLINK_COMMON_EXPORT
    UnionTraits<blink::mojom::FrameTokenDataView, blink::FrameToken> {
 private:
  using DataView = blink::mojom::FrameTokenDataView;

 public:
  static bool Read(DataView input, blink::FrameToken* output);

  static DataView::Tag GetTag(const blink::FrameToken& token) {
    switch (token.variant_index()) {
      case blink::FrameToken::IndexOf<blink::LocalFrameToken>():
        return DataView::Tag::kLocalFrameToken;
      case blink::FrameToken::IndexOf<blink::RemoteFrameToken>():
        return DataView::Tag::kRemoteFrameToken;
    }
    base::ImmediateCrash();
  }

  static const blink::LocalFrameToken& local_frame_token(
      const blink::FrameToken& token) {
    return token.GetAs<blink::LocalFrameToken>();
  }
  static const blink::RemoteFrameToken& remote_frame_token(
      const blink::FrameToken& token) {
    return token.GetAs<blink::RemoteFrameToken>();
  }
};

////////////////////////////////////////////////////////////////////////////////
// WORKER TOKENS

template <>
struct StructTraits<blink::mojom::DedicatedWorkerTokenDataView,
                    blink::DedicatedWorkerToken>
    : public blink::TokenMojomTraitsHelper<
          blink::mojom::DedicatedWorkerTokenDataView,
          blink::DedicatedWorkerToken> {};

template <>
struct StructTraits<blink::mojom::ServiceWorkerTokenDataView,
                    blink::ServiceWorkerToken>
    : public blink::TokenMojomTraitsHelper<
          blink::mojom::ServiceWorkerTokenDataView,
          blink::ServiceWorkerToken> {};

template <>
struct StructTraits<blink::mojom::SharedWorkerTokenDataView,
                    blink::SharedWorkerToken>
    : public blink::TokenMojomTraitsHelper<
          blink::mojom::SharedWorkerTokenDataView,
          blink::SharedWorkerToken> {};

template <>
struct BLINK_COMMON_EXPORT
    UnionTraits<blink::mojom::WorkerTokenDataView, blink::WorkerToken> {
 private:
  using DataView = blink::mojom::WorkerTokenDataView;

 public:
  static bool Read(DataView input, blink::WorkerToken* output);

  static blink::mojom::WorkerTokenDataView::Tag GetTag(
      const blink::WorkerToken& token) {
    switch (token.variant_index()) {
      case blink::WorkerToken::IndexOf<blink::DedicatedWorkerToken>():
        return DataView::Tag::kDedicatedWorkerToken;
      case blink::WorkerToken::IndexOf<blink::ServiceWorkerToken>():
        return DataView::Tag::kServiceWorkerToken;
      case blink::WorkerToken::IndexOf<blink::SharedWorkerToken>():
        return DataView::Tag::kSharedWorkerToken;
    }
    base::ImmediateCrash();
  }

  static const blink::DedicatedWorkerToken& dedicated_worker_token(
      const blink::WorkerToken& token) {
    return token.GetAs<blink::DedicatedWorkerToken>();
  }
  static const blink::ServiceWorkerToken& service_worker_token(
      const blink::WorkerToken& token) {
    return token.GetAs<blink::ServiceWorkerToken>();
  }
  static const blink::SharedWorkerToken& shared_worker_token(
      const blink::WorkerToken& token) {
    return token.GetAs<blink::SharedWorkerToken>();
  }
};

////////////////////////////////////////////////////////////////////////////////
// OTHER TOKENS
//
// Keep this section last.
//
// If you have multiple tokens that make a thematic group, please lift them to
// their own section, in alphabetical order. If adding a new token here, please
// keep the following list in alphabetic order.

template <>
struct BLINK_COMMON_EXPORT
    UnionTraits<blink::mojom::ExecutionContextTokenDataView,
                blink::ExecutionContextToken> {
 private:
  using DataView = blink::mojom::ExecutionContextTokenDataView;

 public:
  static bool Read(blink::mojom::ExecutionContextTokenDataView input,
                   blink::ExecutionContextToken* output);

  static DataView::Tag GetTag(const blink::ExecutionContextToken& token) {
    switch (token.variant_index()) {
      case blink::ExecutionContextToken::IndexOf<blink::LocalFrameToken>():
        return DataView::Tag::kLocalFrameToken;
      case blink::ExecutionContextToken::IndexOf<blink::DedicatedWorkerToken>():
        return DataView::Tag::kDedicatedWorkerToken;
      case blink::ExecutionContextToken::IndexOf<blink::ServiceWorkerToken>():
        return DataView::Tag::kServiceWorkerToken;
      case blink::ExecutionContextToken::IndexOf<blink::SharedWorkerToken>():
        return DataView::Tag::kSharedWorkerToken;
    }
    base::ImmediateCrash();
  }

  static const blink::LocalFrameToken& local_frame_token(
      const blink::ExecutionContextToken& token) {
    return token.GetAs<blink::LocalFrameToken>();
  }
  static const blink::DedicatedWorkerToken& dedicated_worker_token(
      const blink::ExecutionContextToken& token) {
    return token.GetAs<blink::DedicatedWorkerToken>();
  }
  static const blink::ServiceWorkerToken& service_worker_token(
      const blink::ExecutionContextToken& token) {
    return token.GetAs<blink::ServiceWorkerToken>();
  }
  static const blink::SharedWorkerToken& shared_worker_token(
      const blink::ExecutionContextToken& token) {
    return token.GetAs<blink::SharedWorkerToken>();
  }
};

template <>
struct StructTraits<
    blink::mojom::SameDocNavigationScreenshotDestinationTokenDataView,
    blink::SameDocNavigationScreenshotDestinationToken>
    : public blink::TokenMojomTraitsHelper<
          blink::mojom::SameDocNavigationScreenshotDestinationTokenDataView,
          blink::SameDocNavigationScreenshotDestinationToken> {};

template <>
struct StructTraits<blink::mojom::ViewTransitionTokenDataView,
                    blink::ViewTransitionToken>
    : public blink::TokenMojomTraitsHelper<
          blink::mojom::ViewTransitionTokenDataView,
          blink::ViewTransitionToken> {};

template <>
struct BLINK_COMMON_EXPORT
    UnionTraits<blink::mojom::WebGPUExecutionContextTokenDataView,
                blink::WebGPUExecutionContextToken> {
 private:
  using DataView = blink::mojom::WebGPUExecutionContextTokenDataView;

 public:
  static bool Read(DataView input, blink::WebGPUExecutionContextToken* output);

  static DataView::Tag GetTag(const blink::WebGPUExecutionContextToken& token) {
    switch (token.variant_index()) {
      case blink::WebGPUExecutionContextToken::IndexOf<blink::DocumentToken>():
        return DataView::Tag::kDocumentToken;
      case blink::WebGPUExecutionContextToken::IndexOf<
          blink::DedicatedWorkerToken>():
        return DataView::Tag::kDedicatedWorkerToken;
      case blink::WebGPUExecutionContextToken::IndexOf<
          blink::SharedWorkerToken>():
        return DataView::Tag::kSharedWorkerToken;
      case blink::WebGPUExecutionContextToken::IndexOf<
          blink::ServiceWorkerToken>():
        return DataView::Tag::kServiceWorkerToken;
    }
    base::ImmediateCrash();
  }

  static const blink::DocumentToken& document_token(
      const blink::WebGPUExecutionContextToken& token) {
    return token.GetAs<blink::DocumentToken>();
  }
  static const blink::DedicatedWorkerToken& dedicated_worker_token(
      const blink::WebGPUExecutionContextToken& token) {
    return token.GetAs<blink::DedicatedWorkerToken>();
  }
  static const blink::SharedWorkerToken& shared_worker_token(
      const blink::WebGPUExecutionContextToken& token) {
    return token.GetAs<blink::SharedWorkerToken>();
  }
  static const blink::ServiceWorkerToken& service_worker_token(
      const blink::WebGPUExecutionContextToken& token) {
    return token.GetAs<blink::ServiceWorkerToken>();
  }
};

}  // namespace mojo

#endif  // THIRD_PARTY_BLINK_PUBLIC_COMMON_TOKENS_TOKENS_MOJOM_TRAITS_H_
