// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_AUTOPLAY_POLICY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_AUTOPLAY_POLICY_H_

#include "third_party/blink/renderer/core/core_export.h"

namespace blink {

// AutoplayPolicy used to be the class that decided whether a given
// HTMLMediaElement was allowed to play (user-gesture requirements, the
// muted-autoplay-while-visible IntersectionObserver, high-media-engagement
// heuristics, UMA reporting via AutoplayUmaHelper, ...). None of that has
// anything left to decide: HTMLMediaElement never creates a WebMediaPlayer,
// so nothing it does can ever actually play, and Play() always fails with
// NotSupportedError before any autoplay check would run (see
// HTMLMediaElement::Play()). That whole decision engine was deleted along
// with its instance state, static heuristics and AutoplayUmaHelper.
//
// Only this nested enum survives. core/frame/settings.json5 declares
// Settings' `autoplayPolicy` field with `type: "AutoplayPolicy::Type"` and
// `include_paths: ["third_party/blink/renderer/core/html/media/
// autoplay_policy.h"]`, and WebSettingsImpl::SetAutoplayPolicy() casts a
// mojom::blink::AutoplayPolicy into it -- both live in core/frame and
// core/exported, outside this directory's remit, and both need the type to
// keep existing at this exact path and name. Nothing reads the stored value
// any more (its only reader was the deleted AutoplayPolicy instance logic),
// but Settings still needs somewhere to put it.
class CORE_EXPORT AutoplayPolicy {
 public:
  enum class Type {
    kNoUserGestureRequired = 0,
    // A local user gesture on the element is required.
    kUserGestureRequired,
    // The document needs to have received a user activation or received one
    // before navigating.
    kDocumentUserActivationRequired,
  };
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_AUTOPLAY_POLICY_H_
