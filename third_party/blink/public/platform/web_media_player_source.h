// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_PLAYER_SOURCE_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_PLAYER_SOURCE_H_

#include "third_party/blink/public/platform/web_common.h"
#include "third_party/blink/public/platform/web_url.h"

namespace blink {

class BLINK_PLATFORM_EXPORT WebMediaPlayerSource {
 public:
  WebMediaPlayerSource();
  explicit WebMediaPlayerSource(const WebURL&);
  ~WebMediaPlayerSource();

  bool IsURL() const;
  WebURL GetAsURL() const;

  // The MediaStream arm of this union is gone with
  // public/platform/modules/mediastream. HTMLMediaElement still asks, and the
  // answer is now always no -- which is the truth here, not a placeholder:
  // nothing can construct a MediaStream to put in one.
  bool IsMediaStream() const { return false; }

 private:
  WebURL url_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_WEB_MEDIA_PLAYER_SOURCE_H_
