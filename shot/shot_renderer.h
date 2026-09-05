// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_RENDERER_H_
#define SHOT_SHOT_RENDERER_H_

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "shot/shot_bytes.h"
#include "shot/shot_request.h"
#include "third_party/blink/renderer/platform/heap/persistent.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkRefCnt.h"
#include "ui/gfx/geometry/rect.h"
#include "url/gurl.h"

namespace blink {
class Document;
class LocalFrame;
class LocalFrameView;
class Page;
}  // namespace blink

namespace shot {

struct CaptureStats;

// Logs the process's memory at `stage` when SHOT_PROFILE is set; see the
// definition for what it reports.
void LogMemoryStage(const char* stage);

// Whether images' compressed bytes go to a scratch file while the page loads,
// from SHOT_PARK_IMAGES. On by default: on a page with 72 MB of photographs
// it takes 47 MB off the peak and does not show up in the wall clock, because
// the writing happens on a background thread during time the main thread
// spends waiting for the network anyway. Read here and in the runtime, which
// is what opens the file blink parks into.
//
// SHOT_PARK_IMAGES=0 turns parking off -- entirely, because without that file
// blink's disk allocator refuses every write. What it does not do is restore
// what a browser does, which is to park on a two-second delay: this build
// disables that delay in parkable_image.cc, and there is no way to put it
// back at runtime. base::FeatureList::IsEnabled falls through to the
// compiled-in default here, since nothing in this binary ever registers a
// FeatureList; the feature's default *is* the policy. See
// docs/upstream-sync.md, which tracks it as a divergence to re-apply.
bool ParkImagesEnabled();

// One encoded slice of a capture: the image, and where in the document it
// came from. A plain screenshot is exactly one of these covering the whole
// region; a tiles request is several, stacked top to bottom.
struct EncodedTile {
  // CSS pixels, document coordinates -- the same space `clip` is given in.
  gfx::Rect region;
  // The encoded image -- or empty when it was streamed into `path`, which is
  // what happens when the request names one: the file is written a row at a
  // time and the image never exists in memory as a whole.
  Bytes bytes;
  // How many bytes the encoded image is, held or written.
  size_t size = 0;
  std::string path;
};

// Receives each tile as soon as it is encoded. Returning an error abandons the
// capture with that message. Tiles arrive in document order.
using TileSink =
    base::RepeatingCallback<base::expected<void, std::string>(EncodedTile)>;

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

  // Lets go of everything kept between captures for the next one's sake: the
  // page a small capture left attached for the idle turn to detach, and the
  // bitmap it rasters into. For an explicit release of memory, which is the
  // one time keeping either warm is the wrong call.
  void ReleaseRetained();

  // Renders `input` according to `request` and returns the encoded image --
  // in memory, or streamed into `request.path` when that is set, in which
  // case the tile carries the path and size and no bytes.
  //
  // Safe to call more than once on one instance. Each call builds a Page and
  // tears it down before returning, so a resident worker can serve request
  // after request without the previous document's frame still attached.
  base::expected<EncodedTile, std::string> Render(
      RenderInput& input,
      const ScreenshotRequest& request);

  // The same, for a request with `tile` set: the region is rendered in slices
  // of at most `tile.height` CSS pixels, each encoded and handed to `sink` as
  // it is finished. One document load and layout serve every slice.
  base::expected<void, std::string> RenderTiles(RenderInput& input,
                                                const ScreenshotRequest& request,
                                                const TileSink& sink);

 private:
  // Everything Render() and RenderTiles() share: page, document, waiting,
  // lifecycle, then the region in strips through an ImageStream -- one for
  // the whole image, or one per tile. Tiles go to `sink`; a plain screenshot
  // is one tile.
  base::expected<void, std::string> RenderDocument(
      RenderInput& input,
      const ScreenshotRequest& request,
      const TileSink& sink);

  // Scrolls the layout viewport to `top` and re-runs the lifecycle. Returns
  // the offset actually reached, which is clamped by the document's scrollable
  // extent and is what painted coordinates are relative to.
  base::expected<int, std::string> ScrollTo(blink::LocalFrameView* view,
                                            int top);
  // Detaches the frame and drops the page. Idempotent, so the scoped call in
  // Render() and the one in the destructor cannot double-detach.
  void TearDown();

  // TearDown(), but on this thread's next idle turn rather than now, and only
  // if the page attached then is still the one attached now. A small capture
  // has handed its image over by the time this is called; detaching the
  // frame is the one cost left, and nothing the caller is waiting on depends
  // on it. Posted with the page's serial so that a task queued behind a new
  // capture -- a caller that never pumps between requests -- finds a newer
  // page and leaves it alone. Falls back to TearDown() now where there is no
  // task runner to post to.
  void TearDownWhenIdle();
  void TearDownIfStill(uint64_t serial);

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

  // One round of style, layout, prepaint and paint, with the split logged
  // when profiling.
  void RunLifecycle(blink::Document* document, int round);

  // Turns cppgc's collection off for a capture, or back on. Idempotent.
  void SetGarbageCollection(bool enabled);

  // Where in the document the picture comes from, in CSS pixels, honouring
  // selector / clip / fullPage in that order of specificity.
  base::expected<gfx::Rect, std::string> ResolveCaptureRect(
      const ScreenshotRequest& request);

  class ChromeClient;
  class FrameClient;

  blink::Persistent<blink::Page> page_;
  blink::Persistent<blink::LocalFrame> frame_;
  // Whether collection is disabled for the capture in progress; see
  // RenderDocument() and WaitForLoad(). Held as a flag rather than a scope
  // object because the scope is stack-only and WaitForLoad() lifts it.
  bool gc_disabled_ = false;
  // Whether Render() / RenderTiles() hand the raster's freed memory back to
  // the system when RenderDocument() returns. Decided once, on the way out of
  // RenderDocument(): yes for a failed or large capture, no for a small one
  // whose freed spans are cheaper to keep warm for the next request than to
  // decommit and fault back in.
  bool reclaim_after_render_ = true;
  // The bitmap a capture that ImageStream::IsSmall() rasters into, kept from
  // one capture to the next so that the same size need not be allocated and
  // faulted in again, and the strip surfaces its raster threads use, kept
  // the same way. A larger capture resets the bitmap and uses the stream's
  // bounded window instead.
  SkBitmap small_bitmap_;
  std::vector<SkBitmap> small_scratch_;
  // Which page is attached, counting up from CreatePage(); what a deferred
  // TearDownIfStill() checks before detaching.
  uint64_t page_serial_ = 0;
  base::WeakPtrFactory<ShotRenderer> weak_factory_{this};
};

}  // namespace shot

#endif  // SHOT_SHOT_RENDERER_H_
