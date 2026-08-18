// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/platform/web_media_player_source.h"

namespace blink {

WebMediaPlayerSource::WebMediaPlayerSource() = default;

WebMediaPlayerSource::WebMediaPlayerSource(const WebURL& url) : url_(url) {}

WebMediaPlayerSource::~WebMediaPlayerSource() = default;

bool WebMediaPlayerSource::IsURL() const {
  return !url_.IsEmpty();
}

WebURL WebMediaPlayerSource::GetAsURL() const {
  return url_;
}

}  // namespace blink
