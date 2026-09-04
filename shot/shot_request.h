// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_REQUEST_H_
#define SHOT_SHOT_REQUEST_H_

#include <map>
#include <optional>
#include <string>
#include <string_view>

#include "base/types/expected.h"

namespace shot {

// One screenshot request, as it arrives over the wire.
//
// The field names mirror shotium's public ScreenshotOptions exactly, so that
// the JS layer is a transport and not a translation: anything it has to rename
// is somewhere a bug can hide. See docs/shotium-plan.md section 4.
//
// Two fields have no counterpart in ScreenshotOptions as it stands -- `width`
// and `height`. A screenshot needs a viewport and ScreenshotOptions has no way
// to say what it is; fullPage and clip both describe what to take *out* of a
// viewport rather than how big it is. They are here with browser defaults so
// the protocol is complete, and the public interface will need a `viewport`
// field of some shape to reach them.
struct Clip {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;
};

// Split the captured region into horizontal slices, each encoded on its own.
//
// The region is whatever selector / clip / fullPage / the viewport would have
// produced as one image; `height` is the most CSS pixels one slice covers, and
// the last slice is whatever is left. The document is loaded and laid out once
// for all of them -- which is the point, and the reason this is an engine
// feature rather than a loop over `clip` in the caller. Memory is not the
// reason any more: every image, tiled or not, is rastered in strips and
// encoded as it goes, so a tile's height changes only how the result is cut.
struct Tile {
  int height = 0;
};

// The quality used for jpeg and webp when the request did not say. Puppeteer
// leaves it to the encoder default, which for skia's jpeg encoder is 100 -- a
// file three times the size of one at 90 for no difference a screenshot can
// show. 90 is what every screenshot service that documents its default picks.
constexpr int kDefaultLossyQuality = 90;

struct ScreenshotRequest {
  // Required. A URL, or a path that the caller wants treated as one.
  std::string file;

  std::string type = "png";  // png | jpeg | webp
  bool full_page = false;
  std::string selector;
  std::optional<int> quality;  // 1-100, jpeg/webp only
  double scale = 1.0;
  bool omit_background = false;
  std::string path;  // if set, the worker writes the file itself

  int timeout_ms = 30000;
  std::string wait_until = "load";  // load | networkidle

  // What this capture may do with the HTTP cache, spelled the way fetch()
  // spells it: default | reload | no-store | only-if-cached. Turned into
  // net::LOAD_* by CacheModeToLoadFlags and applied to every request the
  // document makes, not just the top-level one -- a reload that refreshed the
  // HTML and reused yesterday's stylesheet would be a confusing thing to have
  // asked for.
  std::string cache = "default";

  // Extra request headers, sent with the document and with the subresources
  // that are same-origin with it. The usual reason is a credential --
  // Authorization, or a Cookie for a session the caller already has -- which
  // is exactly why they stop at the origin boundary.
  //
  // A map rather than net::HttpRequestHeaders because this struct is the wire
  // format and has no business naming a //net type; the conversion happens
  // once, in shot_capture.cc.
  std::map<std::string, std::string> headers;

  std::optional<Clip> clip;

  // Set by a tiles request. A plain screenshot never has it, and a tiles
  // request always does -- the two are different calls with different answers
  // (one image against a list of them), and the field is what tells the
  // engine which was made.
  std::optional<Tile> tile;

  // Viewport. Not in ScreenshotOptions yet; see the comment above.
  int width = 1280;
  int height = 720;

  // Local file access. Off by default because a library should not decide for
  // its caller that a document may read the filesystem it is being rendered on;
  // the CLI turns it on for the file it was pointed at.
  bool allow_file_access = false;
};

// Parses one request. The error is the message the caller sees, so it names the
// field rather than the parser.
//
// `default_allow_file_access` is what an absent `allowFileAccess` means. It is
// a parameter rather than a constant because the answer belongs to whoever
// started the process: the operator knows whether the sender is trusted, and
// the sender may not. A request that states the field wins either way.
base::expected<ScreenshotRequest, std::string> ParseScreenshotRequest(
    std::string_view json,
    bool default_allow_file_access = false);

}  // namespace shot

#endif  // SHOT_SHOT_REQUEST_H_
