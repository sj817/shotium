// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/platform/graphics/decoding_image_generator.h"

#include <array>
#include <cstdint>

#include "base/check_op.h"
#include "base/memory/scoped_refptr.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/platform/graphics/image_frame_generator.h"
#include "third_party/blink/renderer/platform/image-decoders/image_decoder_test_helpers.h"
#include "third_party/blink/renderer/platform/image-decoders/segment_reader.h"
#include "third_party/skia/include/core/SkData.h"
#include "third_party/skia/include/core/SkStream.h"

namespace blink {

namespace {

constexpr unsigned kTooShortForSignature = 5;

class ChunkedSegmentReader final : public SegmentReader {
 public:
  size_t size() const override { return kBytes.size(); }

  base::span<const uint8_t> GetSomeData(size_t position) const override {
    CHECK_GT(lock_depth_, 0);
    if (position < kSplit) {
      return base::span(kBytes).first(kSplit).subspan(position);
    }
    if (position < kBytes.size()) {
      return base::span(kBytes).subspan(position);
    }
    return {};
  }

  sk_sp<const SkData> GetAsSkData() const override {
    ++get_as_sk_data_calls_;
    return nullptr;
  }

  void LockData() override { ++lock_depth_; }
  void UnlockData() override {
    CHECK_GT(lock_depth_, 0);
    --lock_depth_;
  }

  int get_as_sk_data_calls() const { return get_as_sk_data_calls_; }
  int lock_depth() const { return lock_depth_; }

  static constexpr std::array<uint8_t, 6> kBytes = {1, 2, 3, 4, 5, 6};

 private:
  static constexpr size_t kSplit = 3;
  ~ChunkedSegmentReader() override = default;

  mutable int get_as_sk_data_calls_ = 0;
  int lock_depth_ = 0;
};

scoped_refptr<SegmentReader> CreateSegmentReader(
    base::span<uint8_t> reference_data) {
  PrepareReferenceData(reference_data);
  scoped_refptr<SharedBuffer> data = SharedBuffer::Create(reference_data);
  return SegmentReader::CreateFromSharedBuffer(std::move(data));
}

}  // namespace

class DecodingImageGeneratorTest : public testing::Test {};

TEST_F(DecodingImageGeneratorTest, Create) {
  scoped_refptr<SharedBuffer> reference_data =
      ReadFileToSharedBuffer(kDecodersTestingDir, "radient.gif");
  scoped_refptr<SegmentReader> reader =
      SegmentReader::CreateFromSharedBuffer(std::move(reference_data));
  std::unique_ptr<SkImageGenerator> generator =
      DecodingImageGenerator::CreateAsSkImageGenerator(reader->GetAsSkData());
  // Sanity-check the image to make sure it was loaded.
  EXPECT_EQ(generator->getInfo().width(), 32);
  EXPECT_EQ(generator->getInfo().height(), 32);
}

TEST_F(DecodingImageGeneratorTest, CreateWithNoSize) {
  // Construct dummy image data that produces no valid size from the
  // ImageDecoder.
  std::array<uint8_t, kDefaultTestSize> reference_data;
  EXPECT_EQ(nullptr, DecodingImageGenerator::CreateAsSkImageGenerator(
                         CreateSegmentReader(reference_data)->GetAsSkData()));
}

TEST_F(DecodingImageGeneratorTest, CreateWithNullImageDecoder) {
  // Construct dummy image data that will produce a null image decoder
  // due to data being too short for a signature.
  std::array<uint8_t, kTooShortForSignature> reference_data;
  EXPECT_EQ(nullptr, DecodingImageGenerator::CreateAsSkImageGenerator(
                         CreateSegmentReader(reference_data)->GetAsSkData()));
}

// This is a regression test for crbug.com/341812566 and passes if it does not
// crash under ASAN.
TEST_F(DecodingImageGeneratorTest, AdjustedGetPixels) {
  scoped_refptr<SharedBuffer> reference_data =
      ReadFileToSharedBuffer(kDecodersTestingDir, "radient.gif");
  scoped_refptr<SegmentReader> reader =
      SegmentReader::CreateFromSharedBuffer(std::move(reference_data));
  std::unique_ptr<SkImageGenerator> generator =
      DecodingImageGenerator::CreateAsSkImageGenerator(reader->GetAsSkData());
  SkImageInfo info = SkImageInfo::MakeA8(32, 32);
  std::vector<size_t> memory(info.computeMinByteSize());
  EXPECT_TRUE(generator->getPixels(info, memory.data(), info.minRowBytes()));
}

TEST_F(DecodingImageGeneratorTest, EncodedDataStreamDoesNotFlattenSegments) {
  scoped_refptr<ChunkedSegmentReader> reader =
      base::MakeRefCounted<ChunkedSegmentReader>();
  scoped_refptr<ImageFrameGenerator> frame = ImageFrameGenerator::Create(
      SkISize::Make(1, 1), false, ColorBehavior::kIgnore,
      cc::AuxImage::kDefault, {});
  sk_sp<DecodingImageGenerator> generator = DecodingImageGenerator::Create(
      std::move(frame), SkImageInfo::MakeN32Premul(1, 1), gfx::HDRMetadata(),
      reader, {FrameMetadata()}, PaintImage::GetNextContentId(),
      true /* all_data_received */, false /* can_yuv_decode */,
      cc::ImageHeaderMetadata());

  std::unique_ptr<SkStream> stream = generator->GetEncodedDataStream();
  ASSERT_TRUE(stream);
  EXPECT_EQ(1, reader->lock_depth());
  EXPECT_EQ(0, reader->get_as_sk_data_calls());

  std::array<uint8_t, ChunkedSegmentReader::kBytes.size()> bytes;
  EXPECT_EQ(bytes.size(), stream->read(bytes.data(), bytes.size()));
  EXPECT_EQ(ChunkedSegmentReader::kBytes, bytes);
  EXPECT_EQ(0, reader->get_as_sk_data_calls());

  // The stream keeps the source alive and locked independently of the
  // generator, so a codec which already acquired it may finish after discard.
  EXPECT_TRUE(generator->DiscardEncodedData());
  generator.reset();
  EXPECT_TRUE(stream->rewind());
  EXPECT_EQ(bytes.size(), stream->read(bytes.data(), bytes.size()));
  EXPECT_EQ(ChunkedSegmentReader::kBytes, bytes);
  stream.reset();
  EXPECT_EQ(0, reader->lock_depth());
  EXPECT_EQ(0, reader->get_as_sk_data_calls());
}

// TODO(wkorman): Test Create with a null ImageFrameGenerator. We'd
// need a way to intercept construction of the instance (and could do
// same for ImageDecoder above to reduce fragility of knowing a short
// signature will produce a null ImageDecoder). Note that it's not
// clear that it's possible to end up with a null ImageFrameGenerator,
// so maybe we can just remove that check from
// DecodingImageGenerator::Create.

}  // namespace blink
