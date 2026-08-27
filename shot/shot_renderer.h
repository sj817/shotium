// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_RENDERER_H_
#define SHOT_SHOT_RENDERER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "shot/shot_request.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

namespace blink {
class LocalFrame;
class LocalFrameView;
class Page;
}  // namespace blink

namespace shot {

// The document to render, already fetched.
//
// The bytes arrive here rather than being fetched from inside blink because
// LocalFrame::ForceSynchronousDocumentInstall takes bytes: there is no
// navigation and no DocumentLoader in this pipeline, so the top-level document
// is delivered, not loaded. Subresources are a different story -- those do go
// through ResourceFetcher and ShotURLLoader.
struct RenderInput {
  GURL url;
  std::string body;
  // What the parser is chosen by. "text/html" unless a server said otherwise.
  std::string mime_type = "text/html";
  // From the Content-Type header, when there was one. Empty means the document
  // has to declare its own encoding, exactly as in a browser.
  std::string charset;
  // What the server answered, or 0 for a document that came off the disk and
  // had nobody to answer. Carried through to the caller's statistics: a
  // screenshot of a 404 page is a valid screenshot and an easy thing to
  // mistake for the page that was asked for.
  int http_status = 0;
};

// Renders one document to one encoded image, entirely inside Blink.
//
// There is no browser here: no ContentMain, no RenderProcess, no
// RenderFrameHost, no navigation, no aura window, no ui::Compositor, no viz, no
// GPU process, no sandbox. The whole pipeline is
//
//   Page::CreateNonOrdinary + LocalFrame + LocalFrameView
//     -> LocalFrame::ForceSynchronousDocumentInstall("text/html", bytes, url)
//     -> LocalFrameView::UpdateAllLifecyclePhases()   (style, layout, prepaint,
//                                                      paint)
//     -> LocalFrameView::GetPaintRecord()             (the paint phase's output)
//     -> SkiaPaintCanvas over an SkSurface            (CPU raster)
//     -> PNG / JPEG / WebP
//
// This is not a reimplementation of anything. It is the same shape blink
// already uses for SVGImage, which is a full document -- DOM, CSS, layout,
// paint -- that cannot have a renderer process of its own; see
// core/svg/graphics/isolated_svg_document_host.cc. The cc::PaintRecord returned
// by GetPaintRecord() *is* the output of blink's paint phase; replaying it onto
// a bitmap is what the compositor does to a tile, minus the tiling.
class ShotRenderer {
 public:
  ShotRenderer();
  ShotRenderer(const ShotRenderer&) = delete;
  ShotRenderer& operator=(const ShotRenderer&) = delete;
  ~ShotRenderer();

  // Renders `input` according to `request` and returns the encoded image.
  //
  // Safe to call more than once on one instance. Each call builds a Page and
  // tears it down before returning, so a resident worker can serve request
  // after request without the previous document's frame still attached.
  base::expected<std::vector<uint8_t>, std::string> Render(
      const RenderInput& input,
      const ScreenshotRequest& request);

 private:
  // Detaches the frame and drops the page. Idempotent, so the scoped call in
  // Render() and the one in the destructor cannot double-detach.
  void TearDown();

  // Brings up the Page/LocalFrame/Document. Separate from Render() only so the
  // failure points are named.
  base::expected<void, std::string> CreatePage(const RenderInput& input,
                                               const ScreenshotRequest& request);

  // Runs the message loop until the document has loaded, or fails at
  // `timeout`. Without this the image shows the document as it looked in the
  // first instant of its life: fallback text where a web font belongs and
  // broken-image icons where the bitmaps belong.
  //
  // `wait_until` is "load" -- parsing done, load event fired, no request in
  // flight -- or "networkidle", which additionally requires that nothing has
  // been in flight for a continuous quiet window. The two differ only for
  // documents that keep fetching after the load event, which without script
  // means CSS that pulls in more CSS, or fonts a late style change brought in.
  base::expected<void, std::string> WaitForLoad(const std::string& wait_until,
                                                base::TimeDelta timeout);

  // Grows the viewport until the whole document fits in it, so that content
  // below the fold is laid out and painted rather than culled. Returns the
  // final content size in CSS pixels.
  gfx::Size ExpandViewportToFit(int minimum_width, int minimum_height);

  // Where in the document the picture comes from, in CSS pixels, honouring
  // selector / clip / fullPage in that order of specificity.
  base::expected<gfx::Rect, std::string> ResolveCaptureRect(
      const ScreenshotRequest& request);

  class ChromeClient;
  class FrameClient;

  blink::Persistent<blink::Page> page_;
  blink::Persistent<blink::LocalFrame> frame_;
};

}  // namespace shot

#endif  // SHOT_SHOT_RENDERER_H_
