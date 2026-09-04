// Copyright 2026 The Shot Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_image_stream.h"

#include <setjmp.h>
#include <stdio.h>  // jpeglib.h needs FILE.

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <optional>
#include <string_view>
#include <utility>

#include "base/check_op.h"
#include "base/compiler_specific.h"
#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "base/strings/string_number_conversions.h"
#include "base/synchronization/condition_variable.h"
#include "base/synchronization/lock.h"
#include "base/system/sys_info.h"
#include "base/thread_annotations.h"
#include "base/threading/simple_thread.h"
#include "cc/paint/decoded_draw_image.h"
#include "cc/paint/discardable_image_map.h"
#include "cc/paint/display_item_list.h"
#include "cc/paint/draw_image.h"
#include "cc/paint/draw_looper.h"
#include "cc/paint/image_provider.h"
#include "cc/paint/paint_filter.h"
#include "cc/paint/paint_flags.h"
#include "cc/paint/paint_image.h"
#include "cc/paint/paint_op.h"
#include "cc/paint/paint_op_buffer.h"
#include "cc/paint/paint_op_buffer_iterator.h"
#include "cc/paint/paint_record.h"
#include "cc/paint/paint_shader.h"
#include "partition_alloc/page_allocator.h"
#include "partition_alloc/page_allocator_constants.h"
#include "shot/shot_bytes.h"
#include "shot/shot_renderer.h"
#include "shot/shot_request.h"
#include "third_party/libwebp/src/src/webp/encode.h"
#include "third_party/skia/include/codec/SkCodec.h"
#include "third_party/skia/include/codec/SkPngRustDecoder.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "third_party/skia/include/core/SkCanvas.h"
#include "third_party/skia/include/core/SkImage.h"
#include "third_party/skia/include/core/SkImageInfo.h"
#include "third_party/skia/include/core/SkPixmap.h"
#include "third_party/skia/include/core/SkSamplingOptions.h"
#include "third_party/skia/include/core/SkStream.h"
#include "third_party/skia/include/core/SkSurface.h"
#include "third_party/skia/include/encode/SkEncoder.h"
#include "third_party/skia/include/encode/SkPngRustEncoder.h"

extern "C" {
#include "jpeglib.h"
}

namespace shot {

namespace {

// Rows per strip. Even, because WebP's chroma planes are sampled 2x2 and a
// band handed to libwebp has to start on an even row to convert the same way
// a whole picture would. Small, because a strip is the unit of memory in
// flight: at 1440 pixels wide one strip is 1.5 MB.
constexpr int kStripRows = 256;

// The most rows a strip is rastered outside itself, for a document with
// effects that read around the pixel they write -- blurs, drop shadows,
// backdrop filters.
//
// How many rows are actually needed is a property of the page, not a number
// to pick: it is how far the widest effect in the paint reaches, which the
// filters and the loopers will say when asked, and ReadAroundExtent asks
// them. This is only the ceiling on the answer.
//
// It used to be the answer. 128 rows was verified against a single-surface
// raster on a page of 360 backdrop-filter cards, where the two differ by at
// most 5 of 255 in any channel with the differing rows spread through the
// image -- but that page's filters read about 20 pixels. A page with
// `filter: blur(300px)` reads three sigma, or 450 rows, and rasters with a
// difference that starts exactly at a strip boundary, which is what a margin
// that is too small looks like.
//
// A ceiling is still needed because a margin is rows rastered twice, and
// because a filter can report an unbounded reach. Exceeding it is not a
// crash, it is a seam: a page whose effects read further than this rasters
// the way every page did before. What a large margin costs is threads rather
// than memory -- the strip budget is divided by what one thread holds, so a
// page needing 1024 rows of margin simply gets fewer of them.
// SHOT_STRIP_MARGIN moves the ceiling, which is how a suspected seam is
// confirmed: raise it and see whether the difference goes away.
constexpr int kMaxStripMargin = 1024;

// An image up to this many pixels is rastered as one strip, on the calling
// thread. Below this, striping costs more than it parallelises, and one strip
// through one canvas reproduces the single-surface raster byte for byte.
constexpr int64_t kSingleStripPixels = 4 * 1000 * 1000;

// What the strips in flight may hold between them: committed rows plus the
// raster threads' own margin surfaces. Sets the thread count on a wide page.
//
// 32 MB, which is where the curve turns. On a 1440-wide page a strip is
// 1.5 MB, so this allows ten threads. Measured on a 46,000 px page of
// photographs, peak private bytes and wall clock against this number:
//
//   12 MB   83 MB  2322 ms      32 MB   96 MB  2156 ms
//   16 MB   81 MB  2427 ms      48 MB  104 MB  2223 ms
//   24 MB   92 MB  2253 ms
//
// Below 32 the threads run out before the work does and it costs time for
// nothing; above it the extra threads buy no wall clock and each one brings
// its own concurrent decode. SHOT_STRIP_BUDGET_MB moves it either way.
constexpr int64_t kDefaultStripBudgetMb = 32;

int EnvInt(const char* name, int fallback) {
  const char* value = std::getenv(name);
  if (!value || !*value) {
    return fallback;
  }
  int parsed = 0;
  return base::StringToInt(value, &parsed) ? parsed : fallback;
}

// The levers that were put in to save memory, each behind its own switch so
// that its cost in wall-clock time can be measured on its own rather than
// argued about. All default on, because all of them measured free or better;
// the ones that measured expensive are off by default instead (see
// ParkImagesEnabled and SHOT_STRIP_BUDGET_MB).
bool StreamPngEnabled() {
  static const bool enabled = EnvInt("SHOT_STREAM_PNG", 1) != 0;
  return enabled;
}
bool DiscardEncodedEnabled() {
  static const bool enabled = EnvInt("SHOT_DISCARD_ENCODED", 1) != 0;
  return enabled;
}

// Mirrors the same-named helper in shot_renderer.cc: profiling logs from this
// translation unit are gated on SHOT_PROFILE (and --verbose lets LOG(INFO)
// through). Kept file-local because the renderer's is in its anonymous
// namespace and not exported.
bool ProfileEnabled() {
  static const bool enabled = EnvInt("SHOT_PROFILE", 0) != 0;
  return enabled;
}

// A bitmap that exists as address space. The whole image is reserved so that
// an encoder can walk it as one pixmap, but a row is only backed by memory
// between the moment it is about to be rastered and the moment the encoder is
// done with it. Rows are committed and released from the top down, so the
// committed rows are always one contiguous window.
class WindowedBitmap {
 public:
  static std::unique_ptr<WindowedBitmap> Create(const SkImageInfo& info) {
    const size_t row_bytes = info.minRowBytes();
    const size_t bytes = row_bytes * static_cast<size_t>(info.height());
    const size_t granularity =
        partition_alloc::internal::PageAllocationGranularity();
    const size_t reserved =
        (bytes + granularity - 1) / granularity * granularity;
    const uintptr_t base = partition_alloc::AllocPages(
        reserved, granularity,
        partition_alloc::PageAccessibilityConfiguration(
            partition_alloc::PageAccessibilityConfiguration::kInaccessible),
        partition_alloc::PageTag::kChromium);
    if (!base) {
      return nullptr;
    }
    return base::WrapUnique(
        new WindowedBitmap(info, base, reserved, row_bytes));
  }

  WindowedBitmap(const WindowedBitmap&) = delete;
  WindowedBitmap& operator=(const WindowedBitmap&) = delete;

  ~WindowedBitmap() { partition_alloc::FreePages(base_, reserved_); }

  const SkPixmap& pixmap() const { return pixmap_; }

  // Backs rows [first, end) with memory. Pages already committed stay as they
  // are; new ones come back zeroed on every platform this runs on.
  [[nodiscard]] bool CommitRows(int first, int end) {
    const size_t lo = (static_cast<size_t>(first) * row_bytes_) / page_;
    const size_t hi =
        (static_cast<size_t>(end) * row_bytes_ + page_ - 1) / page_;
    if (!has_window_) {
      lo_ = hi_ = lo;
      has_window_ = true;
    }
    if (hi > hi_) {
      const size_t from = std::max(hi_, lo);
      if (!partition_alloc::TryRecommitSystemPages(
              base_ + from * page_, (hi - from) * page_,
              partition_alloc::PageAccessibilityConfiguration(
                  partition_alloc::PageAccessibilityConfiguration::kReadWrite),
              partition_alloc::PageAccessibilityDisposition::kRequireUpdate)) {
        return false;
      }
      hi_ = hi;
      peak_bytes_ = std::max(peak_bytes_, (hi_ - lo_) * page_);
    }
    return true;
  }

  // Gives the pages holding only rows above `end` back to the system.
  void ReleaseRows(int end) {
    const size_t page_end = (static_cast<size_t>(end) * row_bytes_) / page_;
    if (has_window_ && page_end > lo_) {
      partition_alloc::DecommitSystemPages(
          base_ + lo_ * page_, (std::min(page_end, hi_) - lo_) * page_,
          partition_alloc::PageAccessibilityDisposition::kAllowKeepForPerf);
      lo_ = std::min(page_end, hi_);
    }
  }

  size_t peak_bytes() const { return peak_bytes_; }

 private:
  WindowedBitmap(const SkImageInfo& info,
                 uintptr_t base,
                 size_t reserved,
                 size_t row_bytes)
      : base_(base),
        reserved_(reserved),
        row_bytes_(row_bytes),
        page_(partition_alloc::internal::SystemPageSize()),
        pixmap_(info, reinterpret_cast<void*>(base), row_bytes) {}

  const uintptr_t base_;
  const size_t reserved_;
  const size_t row_bytes_;
  const size_t page_;
  SkPixmap pixmap_;
  bool has_window_ = false;
  size_t lo_ = 0;
  size_t hi_ = 0;
  size_t peak_bytes_ = 0;
};

// An SkWStream onto a Bytes::Writer, so that skia's encoders produce what
// the capture returns without a copy at the end.
class BytesStream final : public SkWStream {
 public:
  BytesStream(size_t capacity, base::File file)
      : writer_(capacity, std::move(file)) {}
  bool write(const void* buffer, size_t size) override {
    // SAFETY: SkWStream's contract is that `buffer` holds `size` bytes.
    return writer_.Append(
        UNSAFE_BUFFERS(base::span(static_cast<const uint8_t*>(buffer), size)));
  }
  size_t bytesWritten() const override { return writer_.size(); }
  base::expected<Bytes, std::string> Take() { return writer_.Finish(); }

 private:
  Bytes::Writer writer_;
};

// The most an encoded image of this pixmap can be: a PNG of incompressible
// pixels is the pixels plus a filter byte a row and a little framing, and
// nothing else this produces comes close.
size_t EncodedCapacity(const SkPixmap& whole) {
  const size_t raw = whole.info().computeMinByteSize();
  return raw + raw / 32 + static_cast<size_t>(whole.height()) + (1 << 20);
}

// Takes the image's rows from the top down as they become final.
class RowEncoder {
 public:
  virtual ~RowEncoder() = default;
  // The next `rows` rows of the pixmap are final.
  virtual base::expected<void, std::string> Append(int rows) = 0;
  // Every row has been appended; produce the file -- or, when the encoder
  // was given an open file, write the rest of it there and return nothing.
  virtual base::expected<Bytes, std::string> Finish() = 0;
  // How many rows, from the top, the encoder is finished with: rows below
  // this may be released. PNG and JPEG have consumed a row by the time
  // Append() returns; lossy WebP may hold one back; lossless WebP reads the
  // whole picture at once and consumes nothing until Finish().
  virtual int consumed_rows() const = 0;
  // The encoded image's size once Finish() has run, held or written.
  size_t written() const { return written_; }

 protected:
  size_t written_ = 0;
};

// PNG through skia's encoder, which takes rows incrementally and keeps
// nothing of a row once it has been fed to the codec.
class PngRowEncoder final : public RowEncoder {
 public:
  static base::expected<std::unique_ptr<RowEncoder>, std::string> Make(
      const SkPixmap& whole,
      base::File output) {
    auto encoder =
        base::WrapUnique(new PngRowEncoder(whole, std::move(output)));
    // The same level gfx::PNGCodec::FastEncodeBGRASkBitmap uses, so a page
    // that fits in one strip encodes to the same bytes it did before rows
    // were streamed.
    SkPngRustEncoder::Options options;
    options.fCompressionLevel = SkPngRustEncoder::CompressionLevel::kLow;
    encoder->encoder_ =
        SkPngRustEncoder::Make(&encoder->stream_, encoder->src_, options);
    if (!encoder->encoder_) {
      return base::unexpected("could not start the png encoder");
    }
    return std::unique_ptr<RowEncoder>(std::move(encoder));
  }

  base::expected<void, std::string> Append(int rows) override {
    if (!encoder_->encodeRows(rows)) {
      return base::unexpected("the encoder rejected the image's rows");
    }
    consumed_ += rows;
    return base::ok();
  }

  int consumed_rows() const override { return consumed_; }

  base::expected<Bytes, std::string> Finish() override {
    // Before Take(), not after: finishing a writer hands its bytes away and
    // leaves it reporting nothing written, so reading the size afterwards
    // told every caller the image was zero bytes long.
    written_ = stream_.bytesWritten();
    return stream_.Take();
  }

 private:
  PngRowEncoder(const SkPixmap& whole, base::File output)
      : stream_(EncodedCapacity(whole), std::move(output)), src_(whole) {}

  int consumed_ = 0;
  // SkEncoder keeps a reference to the pixmap it was made with, so this one
  // lives here, before the encoder that refers to it.
  BytesStream stream_;
  SkPixmap src_;
  std::unique_ptr<SkEncoder> encoder_;
};

// JPEG straight through libjpeg-turbo, a row at a time.
//
// Not skia's encoder, because that one always turns on optimize_coding -- a
// second Huffman pass over the whole image, for which libjpeg buffers every
// coefficient: three bytes a pixel, 177 MB for a 1440 x 40944 page, held
// until the last row.
class JpegRowEncoder final : public RowEncoder {
 public:
  static base::expected<std::unique_ptr<RowEncoder>, std::string> Make(
      const SkPixmap& whole,
      int quality,
      base::File output) {
    if (whole.colorType() != kBGRA_8888_SkColorType &&
        whole.colorType() != kRGBA_8888_SkColorType) {
      return base::unexpected("the jpeg encoder needs 32-bit pixels");
    }
    auto encoder =
        base::WrapUnique(new JpegRowEncoder(whole, std::move(output)));
    if (!encoder->Start(quality)) {
      return base::unexpected("could not start the jpeg encoder");
    }
    return std::unique_ptr<RowEncoder>(std::move(encoder));
  }

  ~JpegRowEncoder() override {
    if (started_) {
      jpeg_destroy_compress(&info_);
    }
  }

  base::expected<void, std::string> Append(int rows) override {
    if (setjmp(error_.jump)) {
      return base::unexpected("the jpeg encoder failed on the image's rows");
    }
    for (int r = 0; r < rows; ++r) {
      // libjpeg reads the row and does not write it, but its API predates
      // const.
      JSAMPROW row = static_cast<JSAMPROW>(
          const_cast<void*>(whole_.addr(0, consumed_ + r)));
      if (jpeg_write_scanlines(&info_, &row, 1) != 1) {
        return base::unexpected("the jpeg encoder rejected a row");
      }
    }
    consumed_ += rows;
    return base::ok();
  }

  int consumed_rows() const override { return consumed_; }

  base::expected<Bytes, std::string> Finish() override {
    if (setjmp(error_.jump)) {
      return base::unexpected("the jpeg encoder failed to finish");
    }
    jpeg_finish_compress(&info_);
    written_ = writer_.size();
    return writer_.Finish();
  }

 private:
  struct ErrorManager {
    jpeg_error_mgr pub;
    jmp_buf jump;
  };
  struct Destination {
    jpeg_destination_mgr pub;
    JpegRowEncoder* self;
  };
  static constexpr size_t kChunk = 64 << 10;

  JpegRowEncoder(const SkPixmap& whole, base::File output)
      : whole_(whole), writer_(EncodedCapacity(whole), std::move(output)) {}

  static void OnError(j_common_ptr info) {
    longjmp(reinterpret_cast<ErrorManager*>(info->err)->jump, 1);
  }
  static void OnMessage(j_common_ptr) {}
  static void InitDestination(j_compress_ptr info) {
    auto* self = reinterpret_cast<Destination*>(info->dest)->self;
    self->destination_.pub.next_output_byte = self->chunk_.data();
    self->destination_.pub.free_in_buffer = kChunk;
  }
  static boolean EmptyOutputBuffer(j_compress_ptr info) {
    // libjpeg's contract: the whole buffer is full, regardless of
    // free_in_buffer.
    auto* self = reinterpret_cast<Destination*>(info->dest)->self;
    if (!self->writer_.Append(self->chunk_)) {
      OnError(reinterpret_cast<j_common_ptr>(info));
    }
    InitDestination(info);
    return TRUE;
  }
  static void TermDestination(j_compress_ptr info) {
    auto* self = reinterpret_cast<Destination*>(info->dest)->self;
    const size_t used = kChunk - self->destination_.pub.free_in_buffer;
    if (!self->writer_.Append(base::span(self->chunk_).first(used))) {
      OnError(reinterpret_cast<j_common_ptr>(info));
    }
  }

  bool Start(int quality) {
    chunk_.resize(kChunk);
    info_.err = jpeg_std_error(&error_.pub);
    error_.pub.error_exit = &OnError;
    error_.pub.output_message = &OnMessage;
    if (setjmp(error_.jump)) {
      return false;
    }
    jpeg_create_compress(&info_);
    started_ = true;
    destination_.pub.init_destination = &InitDestination;
    destination_.pub.empty_output_buffer = &EmptyOutputBuffer;
    destination_.pub.term_destination = &TermDestination;
    destination_.self = this;
    info_.dest = &destination_.pub;
    info_.image_width = static_cast<JDIMENSION>(whole_.width());
    info_.image_height = static_cast<JDIMENSION>(whole_.height());
    info_.input_components = 4;
    info_.in_color_space = whole_.colorType() == kBGRA_8888_SkColorType
                               ? JCS_EXT_BGRA
                               : JCS_EXT_RGBA;
    jpeg_set_defaults(&info_);
    jpeg_set_quality(&info_, quality, TRUE);
    // The defaults leave chroma at 4:2:0, which is what skia's encoder and
    // every browser screenshot use. Huffman optimisation is a second pass
    // over every coefficient, buffered whole -- three bytes a pixel -- so a
    // small image takes the smaller file and a large one the smaller
    // process. Measured: 10% smaller files at 1440 x 40944, for 177 MB.
    info_.optimize_coding =
        static_cast<int64_t>(whole_.width()) * whole_.height() <=
                kSingleStripPixels
            ? TRUE
            : FALSE;
    jpeg_start_compress(&info_, TRUE);
    return true;
  }

  const SkPixmap whole_;
  Bytes::Writer writer_;
  std::vector<uint8_t> chunk_;
  ErrorManager error_ = {};
  Destination destination_ = {};
  jpeg_compress_struct info_ = {};
  bool started_ = false;
  int consumed_ = 0;
};

// Row `row` of a libwebp plane, `width` bytes wide.
// SAFETY: a plane libwebp allocated or imported is `stride` bytes a row for
// the picture's height, and every caller stays inside that height.
base::span<uint8_t> PlaneRow(uint8_t* plane, int stride, int row, int width) {
  return UNSAFE_BUFFERS(
      base::span(plane + static_cast<size_t>(row) * static_cast<size_t>(stride),
                 static_cast<size_t>(width)));
}

// libwebp's writer callback, onto a Bytes::Writer in `custom_ptr`.
int WriteWebp(const uint8_t* data, size_t size, const WebPPicture* picture) {
  auto* writer = static_cast<Bytes::Writer*>(picture->custom_ptr);
  // SAFETY: libwebp hands `size` bytes at `data`.
  return writer->Append(UNSAFE_BUFFERS(base::span(data, size))) ? 1 : 0;
}

// Lossy WebP. libwebp encodes from planar YUV, so each band of rows is
// converted as it arrives and only the YUV -- 1.5 bytes a pixel -- accumulates
// until the end. A band starts on an even row, which makes its 2x2 chroma
// blocks the blocks a whole-picture conversion would have formed.
class WebpLossyRowEncoder final : public RowEncoder {
 public:
  static base::expected<std::unique_ptr<RowEncoder>, std::string>
  Make(const SkPixmap& whole, bool opaque, int quality, base::File output) {
    auto encoder = base::WrapUnique(
        new WebpLossyRowEncoder(whole, opaque, quality, std::move(output)));
    if (!WebPPictureInit(&encoder->picture_)) {
      return base::unexpected("libwebp is not the version this was built for");
    }
    encoder->picture_.width = whole.width();
    encoder->picture_.height = whole.height();
    encoder->picture_.use_argb = 0;
    encoder->picture_.colorspace = opaque ? WEBP_YUV420 : WEBP_YUV420A;
    if (!WebPPictureAlloc(&encoder->picture_)) {
      return base::unexpected("could not allocate the WebP picture");
    }
    encoder->allocated_ = true;
    return std::unique_ptr<RowEncoder>(std::move(encoder));
  }

  ~WebpLossyRowEncoder() override {
    if (allocated_) {
      WebPPictureFree(&picture_);
    }
  }

  base::expected<void, std::string> Append(int rows) override {
    // Bands are converted in even-row units. An odd row at the end of a band
    // waits for the next one, unless it is the image's last.
    row_ += rows;
    const bool last = row_ == whole_.height();
    int band_rows = row_ - converted_;
    if (band_rows % 2 != 0 && !last) {
      --band_rows;
    }
    if (band_rows > 0) {
      if (auto converted = Convert(converted_, band_rows);
          !converted.has_value()) {
        return converted;
      }
      converted_ += band_rows;
    }
    return base::ok();
  }

  int consumed_rows() const override { return converted_; }

  base::expected<Bytes, std::string> Finish() override {
    if (converted_ != whole_.height()) {
      return base::unexpected("the WebP encoder did not receive every row");
    }
    WebPConfig config;
    if (!WebPConfigPreset(&config, WEBP_PRESET_DEFAULT,
                          static_cast<float>(quality_))) {
      return base::unexpected("could not configure the WebP encoder");
    }
    // The settings skia's encoder picks for lossy.
    config.lossless = 0;
    config.method = 3;
    Bytes::Writer writer(EncodedCapacity(whole_), std::move(output_));
    picture_.writer = &WriteWebp;
    picture_.custom_ptr = &writer;
    if (!WebPEncode(&config, &picture_)) {
      return base::unexpected("could not encode the image as WebP");
    }
    written_ = writer.size();
    return writer.Finish();
  }

 private:
  WebpLossyRowEncoder(const SkPixmap& whole,
                      bool opaque,
                      int quality,
                      base::File output)
      : whole_(whole),
        opaque_(opaque),
        quality_(quality),
        output_(std::move(output)) {}

  // Converts rows [from, from + rows) into the picture's planes.
  base::expected<void, std::string> Convert(int from, int rows) {
    const int width = whole_.width();
    // libwebp reads RGBA, and premultiplied pixels have to be undone first;
    // one readPixels does both.
    const SkImageInfo band_info = SkImageInfo::Make(
        width, rows, kRGBA_8888_SkColorType,
        opaque_ ? kOpaque_SkAlphaType : kUnpremul_SkAlphaType);
    rgba_.resize(static_cast<size_t>(width) * rows * 4);
    SkPixmap src;
    if (!whole_.extractSubset(&src, SkIRect::MakeXYWH(0, from, width, rows)) ||
        !src.readPixels(band_info, rgba_.data(),
                        static_cast<size_t>(width) * 4)) {
      return base::unexpected("could not read the image's rows for WebP");
    }
    WebPPicture band;
    if (!WebPPictureInit(&band)) {
      return base::unexpected("libwebp is not the version this was built for");
    }
    band.width = width;
    band.height = rows;
    band.use_argb = 0;
    if (!WebPPictureImportRGBA(&band, rgba_.data(), width * 4)) {
      return base::unexpected("could not convert the image's rows for WebP");
    }
    for (int r = 0; r < rows; ++r) {
      PlaneRow(picture_.y, picture_.y_stride, from + r, width)
          .copy_from(PlaneRow(band.y, band.y_stride, r, width));
    }
    const int uv_width = (width + 1) / 2;
    const int uv_rows = (rows + 1) / 2;
    const int uv_from = from / 2;
    for (int r = 0; r < uv_rows; ++r) {
      PlaneRow(picture_.u, picture_.uv_stride, uv_from + r, uv_width)
          .copy_from(PlaneRow(band.u, band.uv_stride, r, uv_width));
      PlaneRow(picture_.v, picture_.uv_stride, uv_from + r, uv_width)
          .copy_from(PlaneRow(band.v, band.uv_stride, r, uv_width));
    }
    if (!opaque_) {
      // A band whose pixels all happened to be opaque was imported without
      // an alpha plane; the picture has one regardless.
      for (int r = 0; r < rows; ++r) {
        base::span<uint8_t> dst =
            PlaneRow(picture_.a, picture_.a_stride, from + r, width);
        if (band.a) {
          dst.copy_from(PlaneRow(band.a, band.a_stride, r, width));
        } else {
          std::ranges::fill(dst, 0xff);
        }
      }
    }
    WebPPictureFree(&band);
    return base::ok();
  }

  const SkPixmap whole_;
  const bool opaque_;
  const int quality_;
  base::File output_;
  WebPPicture picture_ = {};
  bool allocated_ = false;
  int row_ = 0;
  int converted_ = 0;
  std::vector<uint8_t> rgba_;
};

// Lossless WebP. libwebp's lossless encoder needs the whole picture, so this
// is the one encoder that holds every row until the end -- and its own working
// copies on top, which is why lossless is the expensive way to ask for a tall
// page. The rows are handed over in place rather than copied.
class WebpLosslessRowEncoder final : public RowEncoder {
 public:
  WebpLosslessRowEncoder(const SkPixmap& whole,
                         bool opaque,
                         int quality,
                         base::File output)
      : whole_(whole),
        opaque_(opaque),
        quality_(quality),
        output_(std::move(output)) {}

  int consumed_rows() const override { return 0; }

  base::expected<void, std::string> Append(int rows) override {
    // libwebp reads 0xAARRGGBB words: BGRA bytes, unpremultiplied. The rows
    // are ours, so they are converted where they are, a row at a time.
    const SkImageInfo row_info = SkImageInfo::Make(
        whole_.width(), 1, kBGRA_8888_SkColorType,
        opaque_ ? kOpaque_SkAlphaType : kUnpremul_SkAlphaType);
    const bool in_place_ok =
        whole_.colorType() == kBGRA_8888_SkColorType &&
        (opaque_ || whole_.alphaType() != kPremul_SkAlphaType);
    if (!in_place_ok) {
      row_buffer_.resize(row_info.minRowBytes());
      for (int r = row_; r < row_ + rows; ++r) {
        SkPixmap src;
        if (!whole_.extractSubset(&src,
                                  SkIRect::MakeXYWH(0, r, whole_.width(), 1)) ||
            !src.readPixels(row_info, row_buffer_.data(),
                            row_info.minRowBytes())) {
          return base::unexpected(
              "could not convert the image's rows for WebP");
        }
        // SAFETY: `src` is one row of the bitmap, minRowBytes() wide.
        UNSAFE_BUFFERS(base::span(static_cast<uint8_t*>(src.writable_addr()),
                                  row_info.minRowBytes()))
            .copy_from(row_buffer_);
      }
    }
    row_ += rows;
    return base::ok();
  }

  base::expected<Bytes, std::string> Finish() override {
    if (row_ != whole_.height()) {
      return base::unexpected("the WebP encoder did not receive every row");
    }
    WebPConfig config;
    if (!WebPConfigPreset(&config, WEBP_PRESET_DEFAULT,
                          static_cast<float>(quality_))) {
      return base::unexpected("could not configure the WebP encoder");
    }
    // The settings skia's encoder picks for lossless.
    config.lossless = 1;
    config.method = 0;
    WebPPicture picture;
    if (!WebPPictureInit(&picture)) {
      return base::unexpected("libwebp is not the version this was built for");
    }
    picture.width = whole_.width();
    picture.height = whole_.height();
    picture.use_argb = 1;
    picture.argb = static_cast<uint32_t*>(const_cast<void*>(whole_.addr()));
    picture.argb_stride = static_cast<int>(whole_.rowBytes() / 4);
    Bytes::Writer writer(EncodedCapacity(whole_), std::move(output_));
    picture.writer = &WriteWebp;
    picture.custom_ptr = &writer;
    // The picture's pixels are not libwebp's to free; nothing here calls
    // WebPPictureFree because WebPPictureAlloc was never called.
    if (!WebPEncode(&config, &picture)) {
      return base::unexpected("could not encode the image as WebP");
    }
    written_ = writer.size();
    return writer.Finish();
  }

 private:
  const SkPixmap whole_;
  const bool opaque_;
  const int quality_;
  base::File output_;
  int row_ = 0;
  std::vector<uint8_t> row_buffer_;
};

base::expected<std::unique_ptr<RowEncoder>, std::string> MakeRowEncoder(
    const SkPixmap& whole,
    const ScreenshotRequest& request,
    bool opaque,
    int quality,
    base::File output) {
  if (request.type == "png") {
    return PngRowEncoder::Make(whole, std::move(output));
  }
  if (request.type == "jpeg") {
    return JpegRowEncoder::Make(whole, quality, std::move(output));
  }
  if (request.type == "webp") {
    // quality 100 is lossless, as it is for gfx::WebpCodec.
    if (quality >= 100) {
      return std::unique_ptr<RowEncoder>(std::make_unique<WebpLosslessRowEncoder>(
          whole, opaque, quality, std::move(output)));
    }
    return WebpLossyRowEncoder::Make(whole, opaque, quality,
                                     std::move(output));
  }
  return base::unexpected("unsupported image type: " + request.type);
}

// Whether anything in `buffer` reads pixels other than the one it writes:
// filters, shadows and loopers, in the ops themselves or in records nested in
// them. A record without any can be rastered in strips that meet exactly; one
// with them needs the strips to overlap.
// How far outside the pixel it writes one filter reaches, in the space the
// filter is described in.
//
// Asked of a single pixel, so what comes back is the spread itself rather than
// the spread of anything in particular. A filter with unbounded output -- a
// colour filter over the whole plane -- answers with something enormous, which
// the caller clamps; it is not wrong, it just is not a margin anyone can pay.
float FilterOutset(const cc::PaintFilter* filter) {
  if (!filter) {
    return 0;
  }
  const SkIRect spread =
      filter->MapRect(SkIRect::MakeWH(1, 1), /*ctm=*/nullptr,
                      cc::PaintFilter::MapDirection::kForward_MapDirection);
  if (spread.isEmpty()) {
    return 0;
  }
  const int outset = std::max({-spread.top(), spread.bottom() - 1,
                               -spread.left(), spread.right() - 1, 0});
  return static_cast<float>(outset);
}

// How far outside the pixel it writes the widest effect in `buffer` reaches,
// in the buffer's own coordinates. Zero when nothing in it reaches at all,
// which is the common case and the one that needs no margin.
//
// Two different things are being counted. An op that samples the canvas
// around what it writes -- a blur, a backdrop filter -- samples nothing where
// a strip's clip cut its input off. And an op drawn through a looper, which
// is how a box-shadow and a text-shadow are drawn, is culled by cc together
// with the shape it decorates: an SkPaint has no looper, so the fast bounds
// the cull is computed from are the shape's alone and a shadow reaching into
// the strip from a shape above it is dropped. Rows rastered and thrown away
// answer both.
float ReadAroundExtent(const cc::PaintOpBuffer& buffer) {
  float extent = 0;
  for (const cc::PaintOp& op : cc::PaintOpBuffer::Iterator(buffer)) {
    switch (op.GetType()) {
      case cc::PaintOpType::kSaveLayerFilters: {
        const auto& save_layer = static_cast<const cc::SaveLayerFiltersOp&>(op);
        for (const sk_sp<cc::PaintFilter>& filter : save_layer.filters) {
          extent = std::max(extent, FilterOutset(filter.get()));
        }
        extent =
            std::max(extent, FilterOutset(save_layer.backdrop_filter.get()));
        break;
      }
      case cc::PaintOpType::kDrawRecord:
        extent = std::max(
            extent, ReadAroundExtent(
                        static_cast<const cc::DrawRecordOp&>(op).record.buffer()));
        break;
      default:
        break;
    }
    if (!op.IsPaintOpWithFlags()) {
      continue;
    }
    const cc::PaintFlags& flags =
        static_cast<const cc::PaintOpWithFlags&>(op).flags;
    extent = std::max(extent, FilterOutset(flags.getImageFilter().get()));
    if (const sk_sp<cc::DrawLooper>& looper = flags.getLooper()) {
      extent = std::max(extent, looper->MaxOutset());
    }
    if (const cc::PaintShader* shader = flags.getShader();
        shader &&
        shader->shader_type() == cc::PaintShader::Type::kPaintRecord &&
        shader->paint_record()) {
      extent =
          std::max(extent, ReadAroundExtent(shader->paint_record()->buffer()));
    }
  }
  return extent;
}

// One level of downscaling, fed a source row at a time, computing what
// SkPixmap::scalePixels(kLinear) computes for the same sizes: each output
// pixel samples the source at ((x + 0.5) * sw / dw - 0.5,
// (y + 0.5) * sh / dh - 0.5) and blends the four neighbours, edges clamped.
// Only the two source rows around the current output row are ever needed, so
// a level costs two rows of its input rather than all of it.
class RowDownscaler {
 public:
  using Sink = base::RepeatingCallback<void(base::span<const uint32_t>)>;

  RowDownscaler(SkISize from, SkISize to, Sink sink)
      : from_(from),
        to_(to),
        sink_(std::move(sink)),
        prev_(static_cast<size_t>(from.width())),
        curr_(static_cast<size_t>(from.width())),
        out_(static_cast<size_t>(to.width())) {
    const double step = static_cast<double>(from.width()) / to.width();
    x0_.reserve(static_cast<size_t>(to.width()));
    x1_.reserve(static_cast<size_t>(to.width()));
    wx_.reserve(static_cast<size_t>(to.width()));
    for (int x = 0; x < to.width(); ++x) {
      const double s = (x + 0.5) * step - 0.5;
      const int i = static_cast<int>(std::floor(s));
      x0_.push_back(static_cast<size_t>(std::clamp(i, 0, from.width() - 1)));
      x1_.push_back(
          static_cast<size_t>(std::clamp(i + 1, 0, from.width() - 1)));
      wx_.push_back(static_cast<float>(s - i));
    }
  }

  // The next source row, top to bottom.
  void PushRow(base::span<const uint32_t> row) {
    CHECK_EQ(row.size(), curr_.size());
    std::swap(prev_, curr_);
    std::ranges::copy(row, curr_.begin());
    ++rows_;
    Emit();
  }

  // No more source rows: the output rows that sample past the last one take
  // it twice, as a clamped edge does.
  void Finish() {
    finished_ = true;
    Emit();
  }

 private:
  void Emit() {
    const double step = static_cast<double>(from_.height()) / to_.height();
    while (next_y_ < to_.height()) {
      const double s = (next_y_ + 0.5) * step - 0.5;
      const int i = static_cast<int>(std::floor(s));
      const int lo = std::clamp(i, 0, from_.height() - 1);
      const int hi = std::clamp(i + 1, 0, from_.height() - 1);
      if (hi >= rows_ && !finished_) {
        return;
      }
      // Downscaling, so `hi` advances by at least one per output row and the
      // row wanted is always the one just pushed, with `lo` the one before it
      // (or the same one, at a clamped edge).
      const std::vector<uint32_t>& a = lo == rows_ - 1 ? curr_ : prev_;
      const std::vector<uint32_t>& b = hi == rows_ - 1 ? curr_ : prev_;
      CHECK(lo == rows_ - 1 || lo == rows_ - 2);
      CHECK(hi == rows_ - 1 || hi == rows_ - 2);
      Resample(a, b, static_cast<float>(s - i));
      sink_.Run(out_);
      ++next_y_;
    }
  }

  void Resample(const std::vector<uint32_t>& a,
                const std::vector<uint32_t>& b,
                float ty) {
    const auto channel = [](uint32_t px, int shift) {
      return static_cast<float>((px >> shift) & 0xff);
    };
    const auto lerp = [](float p, float q, float t) {
      return p + (q - p) * t;
    };
    for (size_t x = 0; x < out_.size(); ++x) {
      const uint32_t a0 = a[x0_[x]];
      const uint32_t a1 = a[x1_[x]];
      const uint32_t b0 = b[x0_[x]];
      const uint32_t b1 = b[x1_[x]];
      const float tx = wx_[x];
      uint32_t px = 0;
      for (int shift = 0; shift < 32; shift += 8) {
        const float top = lerp(channel(a0, shift), channel(a1, shift), tx);
        const float bottom = lerp(channel(b0, shift), channel(b1, shift), tx);
        const float v = lerp(top, bottom, ty);
        px |= static_cast<uint32_t>(v + 0.5f) << shift;
      }
      out_[x] = px;
    }
  }

  const SkISize from_;
  const SkISize to_;
  const Sink sink_;
  std::vector<uint32_t> prev_;
  std::vector<uint32_t> curr_;
  std::vector<uint32_t> out_;
  std::vector<size_t> x0_;
  std::vector<size_t> x1_;
  std::vector<float> wx_;
  int rows_ = 0;
  int next_y_ = 0;
  bool finished_ = false;
};

// Decodes a static, non-interlaced PNG straight down to the last level that a
// chain of exact halvings reaches above `target`, keeping only a window of
// rows of the full-size decode backed by memory at any moment. The pixels are
// what Decode()'s full decode followed by the same halvings would produce,
// to within rounding, because the same codec writes them and each level
// computes the same bilinear sample; what changes is that the full-size image
// never exists all at once. Returns nullopt for anything else -- JPEG has its
// own scaled decode, an Adam7 PNG is written in passes rather than top to
// bottom, an animated one may want a later frame -- or when there is no level
// to gain.
std::optional<SkBitmap> DecodePngStreaming(const cc::PaintImage& image,
                                           const SkImageInfo& full,
                                           const SkISize& target) {
  if (image.animation_type() != cc::PaintImage::AnimationType::kStatic) {
    return std::nullopt;
  }
  // The levels Decode()'s loop would halve through before its final,
  // fractional step.
  std::vector<SkISize> sizes = {full.dimensions()};
  while ((sizes.back().width() >> 1) >= target.width() &&
         (sizes.back().height() >> 1) >= target.height() &&
         (sizes.back().width() > target.width() ||
          sizes.back().height() > target.height())) {
    sizes.push_back(SkISize::Make(sizes.back().width() >> 1,
                                  sizes.back().height() >> 1));
  }
  if (sizes.size() < 2) {
    return std::nullopt;
  }
  sk_sp<const SkData> data = image.GetEncodedData();
  if (!data || data->size() < 33 ||
      !SkPngRustDecoder::IsPng(data->data(), data->size())) {
    return std::nullopt;
  }
  // SAFETY: SkData owns exactly size() bytes at bytes().
  const base::span<const uint8_t> bytes =
      UNSAFE_BUFFERS(base::span(data->bytes(), data->size()));
  // IHDR is always the first chunk: 8 bytes of signature, 4 of length, 4 of
  // type, 13 of data whose last byte is the interlace method.
  if (bytes.subspan(12u, 4u) != base::as_byte_span(std::string_view("IHDR")) ||
      bytes[28] != 0) {
    return std::nullopt;
  }
  SkCodec::Result result = SkCodec::kSuccess;
  std::unique_ptr<SkCodec> codec = SkPngRustDecoder::Decode(
      SkMemoryStream::MakeDirect(data->data(), data->size()), &result);
  if (!codec || codec->dimensions() != full.dimensions()) {
    return std::nullopt;
  }
  std::unique_ptr<WindowedBitmap> window = WindowedBitmap::Create(full);
  if (!window) {
    return std::nullopt;
  }
  const SkPixmap& src = window->pixmap();
  SkBitmap out;
  if (!out.tryAllocPixels(full.makeDimensions(sizes.back()))) {
    return std::nullopt;
  }

  // The chain, built from the last level back so each stage can be handed the
  // next one's sink.
  int out_row = 0;
  RowDownscaler::Sink next = base::BindRepeating(
      [](SkBitmap* out, int* row, base::span<const uint32_t> pixels) {
        // SAFETY: `out` is `pixels.size()` pixels wide and `*row` counts the
        // rows delivered so far, which the last stage bounds by its height.
        std::ranges::copy(pixels, UNSAFE_BUFFERS(base::span(
                                      out->getAddr32(0, *row), pixels.size()))
                                      .begin());
        ++*row;
      },
      &out, &out_row);
  std::vector<std::unique_ptr<RowDownscaler>> stages(sizes.size() - 1);
  for (size_t level = stages.size(); level-- > 0;) {
    stages[level] = std::make_unique<RowDownscaler>(sizes[level],
                                                    sizes[level + 1], next);
    next = base::BindRepeating(&RowDownscaler::PushRow,
                               base::Unretained(stages[level].get()));
  }

  // About a megabyte of rows per call to the codec.
  const int batch = std::max(
      8, static_cast<int>((size_t{1} << 20) / std::max<size_t>(1, src.rowBytes())));
  codec->setIncrementalRowLimit(batch);
  if (!window->CommitRows(0, std::min(batch, full.height())) ||
      codec->startIncrementalDecode(full, src.writable_addr(),
                                    src.rowBytes()) != SkCodec::kSuccess) {
    return std::nullopt;
  }
  int fed = 0;
  while (true) {
    int rows = 0;
    result = codec->incrementalDecode(&rows);
    if (result == SkCodec::kSuccess) {
      rows = full.height();
    } else if (result != SkCodec::kIncompleteInput || rows <= fed) {
      // A real error, or input that ended early: the codec reports the same
      // row count again rather than more.
      return std::nullopt;
    }
    for (; fed < rows; ++fed) {
      // SAFETY: the codec has written rows [0, rows) of a bitmap
      // full.width() pixels wide, and `fed` < rows.
      stages[0]->PushRow(UNSAFE_BUFFERS(
          base::span(src.addr32(0, fed), static_cast<size_t>(full.width()))));
    }
    if (result == SkCodec::kSuccess) {
      break;
    }
    // The chain keeps its own copies of what it still needs.
    window->ReleaseRows(fed);
    if (!window->CommitRows(fed, std::min(fed + batch, full.height()))) {
      return std::nullopt;
    }
  }
  for (std::unique_ptr<RowDownscaler>& stage : stages) {
    stage->Finish();
  }
  if (out_row != out.height()) {
    return std::nullopt;
  }
  return out;
}

// The images a slice draws, decoded once each at the size they are drawn at
// and held only while a strip that draws them is still to come.
//
// This is the part of cc's software image decode cache a screenshot needs.
// Without it every strip that touches an image asks skia for the pixels,
// skia decodes the whole image at its full size, and its cache keeps it for
// as long as it likes -- a page of thirty photographs held 265 MB of decoded
// pixels until the capture was over.
class DecodedImages final : public cc::ImageProvider {
 public:
  explicit DecodedImages(ImageStreamStats* stats) : stats_(stats) {}
  ~DecodedImages() override = default;

  // The last strip that draws each image. An image not listed is kept until
  // `fallback`. `encoded_sources` holds a PaintImage per drawn image whose
  // compressed bytes may go once that strip is encoded; the caller leaves out
  // the images a later slice still draws.
  void SetLastStrips(base::flat_map<cc::PaintImage::Id, int> last_strips,
                     int fallback,
                     std::map<cc::PaintImage::Id, std::vector<cc::PaintImage>>
                         encoded_sources) {
    base::AutoLock lock(lock_);
    last_strips_ = std::move(last_strips);
    fallback_last_strip_ = fallback;
    encoded_sources_ = std::move(encoded_sources);
  }

  ScopedResult GetRasterContent(const cc::DrawImage& draw_image) override {
    const cc::PaintImage& image = draw_image.paint_image();
    if (image.IsPaintWorklet()) {
      return ScopedResult();
    }
    if (!image.IsLazyGenerated()) {
      return ScopedResult(cc::DecodedDrawImage(
          image.GetSwSkImage(), nullptr, SkSize::Make(0, 0),
          SkSize::Make(1.f, 1.f), draw_image.filter_quality(), true));
    }

    // The size to decode to: the largest power-of-two reduction of the image
    // that is still at least as big as it is drawn -- cc's mip level. A JPEG
    // decodes straight to it; anything else decodes whole and is scaled down
    // once, so that what is held is the size that is drawn.
    const int full_width = image.width();
    const int full_height = image.height();
    int level = 0;
    if (draw_image.filter_quality() != cc::PaintFlags::FilterQuality::kNone &&
        draw_image.matrix_is_decomposable()) {
      const float sx = std::abs(draw_image.scale().width());
      const float sy = std::abs(draw_image.scale().height());
      while (level < 16 && (full_width >> (level + 1)) >= 1 &&
             (full_height >> (level + 1)) >= 1 &&
             static_cast<float>(full_width >> (level + 1)) >=
                 static_cast<float>(full_width) * sx &&
             static_cast<float>(full_height >> (level + 1)) >=
                 static_cast<float>(full_height) * sy) {
        ++level;
      }
    }
    const SkISize target = SkISize::Make(std::max(1, full_width >> level),
                                         std::max(1, full_height >> level));
    const Key key{image.stable_id(), draw_image.frame_index(), target.width(),
                  target.height()};

    base::AutoLock lock(lock_);
    Entry& entry = entries_[key];
    while (entry.decoding) {
      cv_.Wait();
    }
    if (!entry.image && !entry.failed) {
      entry.decoding = true;
      Decoded decoded;
      {
        base::AutoUnlock unlock(lock_);
        decoded = Decode(image, draw_image.frame_index(), target);
      }
      entry.decoding = false;
      if (decoded.image) {
        entry.image = std::move(decoded.image);
        entry.scale_adjustment = decoded.scale_adjustment;
        entry.bytes = decoded.bytes;
        bytes_ += entry.bytes;
        stats_->peak_decoded_bytes =
            std::max(stats_->peak_decoded_bytes, bytes_);
      } else {
        entry.failed = true;
      }
      stats_->decode += decoded.elapsed;
      cv_.Broadcast();
    }
    if (!entry.image) {
      return ScopedResult();
    }
    ++entry.refs;
    return ScopedResult(
        cc::DecodedDrawImage(entry.image, nullptr, SkSize::Make(0, 0),
                             entry.scale_adjustment,
                             draw_image.filter_quality(), true),
        base::BindOnce(&DecodedImages::Unref, base::Unretained(this), key));
  }

  // Strip `strip` and everything above it has been encoded, so an image
  // whose last strip that was can go.
  void StripEncoded(int strip) {
    std::vector<cc::PaintImage> sources_to_discard;
    {
      base::AutoLock lock(lock_);
      encoded_through_ = strip;
      for (auto it = entries_.begin(); it != entries_.end();) {
        if (it->second.refs == 0 && !it->second.decoding &&
            LastStrip(it->first.id) <= strip) {
          bytes_ -= it->second.bytes;
          it = entries_.erase(it);
        } else {
          ++it;
        }
      }
      for (auto it = encoded_sources_.begin();
           DiscardEncodedEnabled() && it != encoded_sources_.end();) {
        const cc::PaintImage::Id id = it->first;
        const bool decoded_pixels_remain = std::ranges::any_of(
            entries_, [id](const auto& pair) { return pair.first.id == id; });
        if (LastStrip(id) <= strip && !decoded_pixels_remain) {
          for (cc::PaintImage& image : it->second) {
            sources_to_discard.push_back(std::move(image));
          }
          it = encoded_sources_.erase(it);
        } else {
          ++it;
        }
      }
    }
    int discarded = 0;
    for (const cc::PaintImage& image : sources_to_discard) {
      discarded += image.DiscardEncodedData();
    }
    if (discarded) {
      // No reclaim here, though freed is not the same as returned and
      // PartitionAlloc does keep a freed slot span committed against the next
      // allocation of its size. Measured on a 46,000 px page in six tiles,
      // reclaiming after each strip that discarded something cost 475 ms of
      // 2700 and took one megabyte off the peak: the raster allocates strips
      // and decodes continuously, so the spans this hands back are wanted
      // again a few milliseconds later. The reclaim that pays for itself is
      // the one after the page is detached, where what is freed is not going
      // to be asked for again.
      if (ProfileEnabled()) {
        LOG(INFO) << "shot: discarded encoded data for " << discarded
                  << " image generator(s) after strip " << strip;
      }
    }
  }

 private:
  struct Key {
    cc::PaintImage::Id id;
    size_t frame;
    int width;
    int height;
    auto operator<=>(const Key&) const = default;
  };
  struct Entry {
    sk_sp<SkImage> image;
    SkSize scale_adjustment = SkSize::Make(1.f, 1.f);
    size_t bytes = 0;
    int refs = 0;
    bool decoding = false;
    bool failed = false;
  };
  struct Decoded {
    sk_sp<SkImage> image;
    SkSize scale_adjustment = SkSize::Make(1.f, 1.f);
    size_t bytes = 0;
    base::TimeDelta elapsed;
  };

  int LastStrip(cc::PaintImage::Id id) const EXCLUSIVE_LOCKS_REQUIRED(lock_) {
    auto it = last_strips_.find(id);
    return it == last_strips_.end() ? fallback_last_strip_ : it->second;
  }

  static Decoded Decode(const cc::PaintImage& image,
                        size_t frame_index,
                        const SkISize& target) {
    Decoded result;
    const base::TimeTicks started = base::TimeTicks::Now();
    const SkISize decode_size = image.GetSupportedDecodeSize(target);
    const SkImageInfo info = SkImageInfo::MakeN32(
        decode_size.width(), decode_size.height(), kPremul_SkAlphaType,
        image.GetSkImageInfo().refColorSpace());
    SkBitmap bitmap;
    std::optional<SkBitmap> streamed =
        frame_index == 0 && StreamPngEnabled()
            ? DecodePngStreaming(image, info, target)
            : std::nullopt;
    if (streamed.has_value()) {
      bitmap = std::move(*streamed);
    } else {
      if (!bitmap.tryAllocPixels(info)) {
        LOG(WARNING) << "shot: could not allocate " << decode_size.width()
                     << "x" << decode_size.height() << " to decode an image";
        return result;
      }
      if (!image.Decode(bitmap.pixmap(), frame_index, cc::AuxImage::kDefault,
                        cc::PaintImage::kDefaultGeneratorClientId)) {
        LOG(WARNING) << "shot: an image failed to decode";
        return result;
      }
    }
    // Down to the target by halving, a level at a time. Each step is a
    // bilinear sample at exactly 0.5 -- a box filter over four pixels -- which
    // is what a mip level is, without asking skia for a whole mipmap chain
    // (a third of the image again) to get there.
    while (bitmap.width() > target.width() ||
           bitmap.height() > target.height()) {
      const SkISize next =
          SkISize::Make(std::max(target.width(), bitmap.width() >> 1),
                        std::max(target.height(), bitmap.height() >> 1));
      SkBitmap scaled;
      if (!scaled.tryAllocPixels(info.makeDimensions(next))) {
        LOG(WARNING) << "shot: could not allocate " << next.width() << "x"
                     << next.height() << " to scale an image";
        return result;
      }
      if (!bitmap.pixmap().scalePixels(
              scaled.pixmap(),
              SkSamplingOptions(SkFilterMode::kLinear, SkMipmapMode::kNone))) {
        LOG(WARNING) << "shot: an image failed to scale";
        return result;
      }
      bitmap = std::move(scaled);
    }
    if (image.GetAlphaType() == kOpaque_SkAlphaType) {
      bitmap.setAlphaType(kOpaque_SkAlphaType);
    }
    bitmap.setImmutable();
    result.image = SkImages::RasterFromBitmap(bitmap);
    result.scale_adjustment = SkSize::Make(
        static_cast<float>(target.width()) / static_cast<float>(image.width()),
        static_cast<float>(target.height()) /
            static_cast<float>(image.height()));
    result.bytes = bitmap.computeByteSize();
    result.elapsed = base::TimeTicks::Now() - started;
    return result;
  }

  void Unref(Key key) {
    base::AutoLock lock(lock_);
    auto it = entries_.find(key);
    if (it == entries_.end()) {
      return;
    }
    if (--it->second.refs == 0 && LastStrip(key.id) <= encoded_through_) {
      bytes_ -= it->second.bytes;
      entries_.erase(it);
    }
  }

  const raw_ptr<ImageStreamStats> stats_;
  base::Lock lock_;
  base::ConditionVariable cv_{&lock_};
  std::map<Key, Entry> entries_ GUARDED_BY(lock_);
  base::flat_map<cc::PaintImage::Id, int> last_strips_ GUARDED_BY(lock_);
  std::map<cc::PaintImage::Id, std::vector<cc::PaintImage>> encoded_sources_
      GUARDED_BY(lock_);
  int fallback_last_strip_ GUARDED_BY(lock_) = 0;
  int encoded_through_ GUARDED_BY(lock_) = -1;
  size_t bytes_ GUARDED_BY(lock_) = 0;
};

// One slice's raster, shared between the threads that raster its strips and
// the thread that encodes them.
struct SliceJob {
  scoped_refptr<const cc::DisplayItemList> list;
  raw_ptr<DecodedImages> images = nullptr;
  raw_ptr<WindowedBitmap> bitmap = nullptr;
  SkSurfaceProps props;
  gfx::Rect cull_rect;
  double scale = 1.0;
  // Image rows [first_row, first_row + rows) in strips of strip_rows.
  int first_row = 0;
  int rows = 0;
  int strip_rows = 0;
  int margin = 0;
  int strips = 0;

  int StripTop(int strip) const { return first_row + strip * strip_rows; }
  int StripBottom(int strip) const {
    return std::min(first_row + rows, StripTop(strip) + strip_rows);
  }

  base::Lock lock;
  base::ConditionVariable cv{&lock};
  // Strips below `allowed` may be taken; the encoder raises it as it frees
  // rows, which is what bounds the memory in flight.
  int next GUARDED_BY(lock) = 0;
  int allowed GUARDED_BY(lock) = 0;
  std::vector<uint8_t> done GUARDED_BY(lock);
  bool failed GUARDED_BY(lock) = false;
  std::string error GUARDED_BY(lock);
};

void DrawSlice(const SliceJob& job, SkCanvas* canvas, int top) {
  canvas->translate(0.0f, static_cast<float>(-top));
  if (job.scale != 1.0) {
    canvas->scale(static_cast<float>(job.scale), static_cast<float>(job.scale));
  }
  if (job.cull_rect.x() != 0 || job.cull_rect.y() != 0) {
    canvas->translate(static_cast<float>(-job.cull_rect.x()),
                      static_cast<float>(-job.cull_rect.y()));
  }
  job.list->Raster(canvas, job.images);
}

// Rasters strip `strip` into the bitmap. `scratch` is the caller's margin
// surface, made on first use and sized for the tallest strip.
base::expected<void, std::string> RasterStrip(const SliceJob& job,
                                              int strip,
                                              sk_sp<SkSurface>& scratch) {
  const int y0 = job.StripTop(strip);
  const int y1 = job.StripBottom(strip);
  const SkPixmap& whole = job.bitmap->pixmap();
  SkPixmap target;
  if (!whole.extractSubset(&target,
                           SkIRect::MakeLTRB(0, y0, whole.width(), y1))) {
    return base::unexpected("could not address the image's rows");
  }
  if (job.margin == 0) {
    std::unique_ptr<SkCanvas> canvas = SkCanvas::MakeRasterDirect(
        target.info(), target.writable_addr(), target.rowBytes(), &job.props);
    if (!canvas) {
      return base::unexpected("could not draw on the image's rows");
    }
    canvas->clear(SK_ColorTRANSPARENT);
    DrawSlice(job, canvas.get(), y0);
    return base::ok();
  }

  const int top = std::max(job.first_row, y0 - job.margin);
  const int bottom = std::min(job.first_row + job.rows, y1 + job.margin);
  if (!scratch) {
    scratch = SkSurfaces::Raster(
        whole.info().makeWH(whole.width(), job.strip_rows + 2 * job.margin),
        &job.props);
    if (!scratch) {
      return base::unexpected("could not allocate a strip surface");
    }
  }
  SkCanvas* canvas = scratch->getCanvas();
  canvas->restoreToCount(1);
  canvas->resetMatrix();
  canvas->clear(SK_ColorTRANSPARENT);
  canvas->save();
  canvas->clipRect(SkRect::MakeWH(static_cast<float>(whole.width()),
                                  static_cast<float>(bottom - top)));
  DrawSlice(job, canvas, top);
  canvas->restore();
  SkPixmap source;
  if (!scratch->peekPixels(&source)) {
    return base::unexpected("could not read the strip surface");
  }
  // Same colour and alpha type on both sides, so this is a row copy.
  if (!source.readPixels(target.info(), target.writable_addr(),
                         target.rowBytes(), 0, y0 - top)) {
    return base::unexpected("could not copy a strip into the image");
  }
  return base::ok();
}

class StripWorker final : public base::DelegateSimpleThread::Delegate {
 public:
  explicit StripWorker(SliceJob& job) : job_(job) {}

  void Run() override {
    sk_sp<SkSurface> scratch;
    for (;;) {
      int strip = -1;
      {
        base::AutoLock lock(job_->lock);
        while (!job_->failed && job_->next >= job_->allowed &&
               job_->next < job_->strips) {
          job_->cv.Wait();
        }
        if (job_->failed || job_->next >= job_->strips) {
          return;
        }
        strip = job_->next++;
      }
      auto rastered = RasterStrip(*job_, strip, scratch);
      base::AutoLock lock(job_->lock);
      if (!rastered.has_value()) {
        if (!job_->failed) {
          job_->failed = true;
          job_->error = rastered.error();
        }
      } else {
        job_->done[strip] = 1;
      }
      job_->cv.Broadcast();
    }
  }

 private:
  const raw_ref<SliceJob> job_;
};

}  // namespace

class ImageStream::Impl {
 public:
  Impl(std::unique_ptr<WindowedBitmap> bitmap,
       std::unique_ptr<RowEncoder> encoder,
       const SkSurfaceProps& props,
       ImageStreamStats* stats)
      : bitmap_(std::move(bitmap)),
        encoder_(std::move(encoder)),
        props_(props),
        stats_(stats) {}

  base::expected<void, std::string> AddSlice(
      scoped_refptr<const cc::DisplayItemList> list,
      const gfx::Rect& cull_rect,
      double scale,
      int device_top,
      int rows,
      const base::flat_set<cc::PaintImage::Id>& keep_encoded) {
    const SkPixmap& whole = bitmap_->pixmap();
    if (device_top != next_row_ || rows <= 0 ||
        device_top + rows > whole.height()) {
      return base::unexpected("slices must tile the image from the top down");
    }
    const base::TimeTicks started = base::TimeTicks::Now();

    SliceJob job;
    job.list = std::move(list);
    job.bitmap = bitmap_.get();
    job.props = props_;
    job.cull_rect = cull_rect;
    job.scale = scale;
    job.first_row = device_top;
    job.rows = rows;
    const bool whole_image = device_top == 0 && rows == whole.height();
    // SHOT_SINGLE_STRIP=1 takes this path whatever the size, which is what
    // makes a strip's raster checkable: one canvas over every row is the
    // answer the strips are trying to reproduce, so a page rendered both ways
    // and compared says whether they do. It is a reference, not an option --
    // it holds the whole image at once, which is the thing this class exists
    // not to do.
    const bool one_strip =
        whole_image &&
        (static_cast<int64_t>(whole.width()) * rows <= kSingleStripPixels ||
         EnvInt("SHOT_SINGLE_STRIP", 0) != 0);
    job.strip_rows = one_strip ? rows : kStripRows;
    job.strips = (rows + job.strip_rows - 1) / job.strip_rows;
    // The paint's own reach, in device rows: it is measured in the list's
    // coordinates, and the list is rastered through `scale`.
    job.margin = 0;
    if (!one_strip) {
      const double reach =
          ReadAroundExtent(job.list->paint_op_buffer()) * scale;
      job.margin = static_cast<int>(
          std::clamp(std::ceil(reach), 0.0,
                     static_cast<double>(EnvInt("SHOT_STRIP_MARGIN",
                                                kMaxStripMargin))));
    }

    // Threads: as many as there are cores and strips, within what the strips
    // in flight may hold. A strip costs its rows in the bitmap, plus a margin
    // surface per thread when there is a margin.
    const int64_t strip_bytes =
        static_cast<int64_t>(whole.rowBytes()) * job.strip_rows;
    const int64_t per_thread =
        strip_bytes + (job.margin ? static_cast<int64_t>(whole.rowBytes()) *
                                        (job.strip_rows + 2 * job.margin)
                                  : 0);
    const int64_t strip_budget =
        static_cast<int64_t>(std::max(
            1, EnvInt("SHOT_STRIP_BUDGET_MB",
                      static_cast<int>(kDefaultStripBudgetMb))))
        << 20;
    const int by_budget = static_cast<int>(std::max<int64_t>(
        1, strip_budget / std::max<int64_t>(1, 2 * per_thread)));
    const int max_threads =
        EnvInt("SHOT_RASTER_THREADS", base::SysInfo::NumberOfProcessors());
    const int threads =
        std::clamp(std::min({job.strips, max_threads, by_budget}), 1, 16);
    // Strips that may be rastered ahead of the encoder.
    const int lookahead = one_strip ? 1 : threads + 2;

    DecodedImages images(stats_);
    job.images = &images;
    {
      // Which strip last draws each image, from where the images are drawn.
      base::flat_map<cc::PaintImage::Id, int> last_strips;
      std::map<cc::PaintImage::Id, std::vector<cc::PaintImage>> encoded_sources;
      scoped_refptr<cc::DiscardableImageMap> map =
          job.list->GenerateDiscardableImageMap(cc::ScrollOffsetMap());
      if (map && !map->empty()) {
        const gfx::Rect slice_css(
            cull_rect.x(),
            cull_rect.y() + static_cast<int>(std::floor(device_top / scale)),
            cull_rect.width(), static_cast<int>(std::ceil(rows / scale)) + 1);
        for (const cc::DrawImage* draw :
             map->GetDiscardableImagesInRect(slice_css)) {
          const cc::PaintImage::Id id = draw->paint_image().stable_id();
          if (!keep_encoded.contains(id)) {
            encoded_sources[id].push_back(draw->paint_image());
          }
          if (last_strips.contains(id)) {
            continue;
          }
          int bottom_css = 0;
          for (const gfx::Rect& rect : map->GetRectsForImage(id)) {
            bottom_css = std::max(bottom_css, rect.bottom());
          }
          const int bottom_row =
              static_cast<int>(std::ceil((bottom_css - cull_rect.y()) * scale));
          const int last =
              std::clamp((bottom_row - 1 - device_top) / job.strip_rows, 0,
                         job.strips - 1);
          last_strips[id] = last;
        }
      }
      images.SetLastStrips(std::move(last_strips), job.strips - 1,
                           std::move(encoded_sources));
    }

    {
      base::AutoLock lock(job.lock);
      job.done.assign(static_cast<size_t>(job.strips), 0);
      job.allowed = std::min(job.strips, lookahead);
      if (!bitmap_->CommitRows(job.StripTop(0),
                               job.StripBottom(job.allowed - 1))) {
        return base::unexpected("could not commit memory for the image's rows");
      }
    }

    std::vector<std::unique_ptr<StripWorker>> delegates;
    std::vector<std::unique_ptr<base::DelegateSimpleThread>> pool;
    for (int i = 0; i < threads; ++i) {
      delegates.push_back(std::make_unique<StripWorker>(job));
      pool.push_back(std::make_unique<base::DelegateSimpleThread>(
          delegates.back().get(), "ShotRaster"));
      pool.back()->Start();
    }
    auto stop_pool = [&](std::string error) {
      {
        base::AutoLock lock(job.lock);
        job.failed = true;
        job.cv.Broadcast();
      }
      for (auto& thread : pool) {
        thread->Join();
      }
      return base::unexpected(std::move(error));
    };

    base::TimeDelta encoding;
    for (int strip = 0; strip < job.strips; ++strip) {
      {
        base::AutoLock lock(job.lock);
        while (!job.failed && !job.done[strip]) {
          job.cv.Wait();
        }
        if (job.failed) {
          std::string error = job.error;
          base::AutoUnlock unlock(job.lock);
          return stop_pool(std::move(error));
        }
      }
      const base::TimeTicks encode_started = base::TimeTicks::Now();
      auto appended =
          encoder_->Append(job.StripBottom(strip) - job.StripTop(strip));
      encoding += base::TimeTicks::Now() - encode_started;
      if (!appended.has_value()) {
        return stop_pool(appended.error());
      }
      bitmap_->ReleaseRows(encoder_->consumed_rows());
      images.StripEncoded(strip);
      if (strip % 20 == 0) {
        LogMemoryStage(("strip " + base::NumberToString(
                                       job.first_row / job.strip_rows + strip))
                           .c_str());
      }
      base::AutoLock lock(job.lock);
      if (job.allowed < job.strips) {
        if (!bitmap_->CommitRows(job.StripTop(job.allowed),
                                 job.StripBottom(job.allowed))) {
          base::AutoUnlock unlock(job.lock);
          return stop_pool("could not commit memory for the image's rows");
        }
        ++job.allowed;
      }
      job.cv.Broadcast();
    }
    for (auto& thread : pool) {
      thread->Join();
    }

    next_row_ += rows;
    stats_->encode += encoding;
    stats_->raster += base::TimeTicks::Now() - started - encoding;
    stats_->strips += job.strips;
    stats_->threads = std::max(stats_->threads, threads);
    stats_->peak_window_bytes =
        std::max(stats_->peak_window_bytes, bitmap_->peak_bytes());
    return base::ok();
  }

  base::expected<Bytes, std::string> Finish() {
    if (next_row_ != bitmap_->pixmap().height()) {
      return base::unexpected("the image's rows were not all rastered");
    }
    const base::TimeTicks started = base::TimeTicks::Now();
    auto out = encoder_->Finish();
    stats_->encode += base::TimeTicks::Now() - started;
    stats_->encoded_bytes = encoder_->written();
    return out;
  }

 private:
  std::unique_ptr<WindowedBitmap> bitmap_;
  std::unique_ptr<RowEncoder> encoder_;
  const SkSurfaceProps props_;
  const raw_ptr<ImageStreamStats> stats_;
  int next_row_ = 0;
};

// static
base::expected<std::unique_ptr<ImageStream>, std::string> ImageStream::Create(
    int width,
    int height,
    const ScreenshotRequest& request,
    bool opaque,
    const SkSurfaceProps& props,
    base::File output) {
  std::unique_ptr<WindowedBitmap> bitmap =
      WindowedBitmap::Create(SkImageInfo::MakeN32Premul(width, height));
  if (!bitmap) {
    return base::unexpected("could not reserve a " +
                            base::NumberToString(width) + "x" +
                            base::NumberToString(height) + " bitmap");
  }
  // The encoder sees the whole image as one pixmap, opaque when the caller
  // promised it, so that PNG writes RGB and WebP skips the alpha plane.
  SkPixmap whole = bitmap->pixmap();
  if (opaque) {
    whole.reset(whole.info().makeAlphaType(kOpaque_SkAlphaType), whole.addr(),
                whole.rowBytes());
  }
  auto encoder =
      MakeRowEncoder(whole, request, opaque,
                     request.quality.value_or(kDefaultLossyQuality),
                     std::move(output));
  if (!encoder.has_value()) {
    return base::unexpected(encoder.error());
  }
  auto stream = base::WrapUnique(new ImageStream(nullptr));
  stream->impl_ = std::make_unique<Impl>(std::move(bitmap), std::move(*encoder),
                                         props, &stream->stats_);
  return stream;
}

ImageStream::ImageStream(std::unique_ptr<Impl> impl) : impl_(std::move(impl)) {}

ImageStream::~ImageStream() = default;

base::expected<void, std::string> ImageStream::AddSlice(
    scoped_refptr<const cc::DisplayItemList> list,
    const gfx::Rect& cull_rect,
    double scale,
    int device_top,
    int rows,
    const base::flat_set<cc::PaintImage::Id>& keep_encoded) {
  return impl_->AddSlice(std::move(list), cull_rect, scale, device_top, rows,
                         keep_encoded);
}

base::expected<Bytes, std::string> ImageStream::Finish() {
  return impl_->Finish();
}

}  // namespace shot
