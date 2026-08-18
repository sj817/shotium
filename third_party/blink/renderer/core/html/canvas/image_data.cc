/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/html/canvas/image_data.h"

#include "third_party/blink/renderer/bindings/core/v8/v8_union_float16array_float32array_uint8clampedarray.h"
#include "third_party/blink/renderer/core/html/canvas/predefined_color_space.h"
#include "third_party/blink/renderer/platform/runtime_enabled_features.h"

namespace blink {

namespace {

// Upper bound on the backing store an ImageData may allocate. This used to be
// the typed-array byte-length ceiling imposed by the script engine. There is
// no script engine, so the ceiling is instead the one this file already
// enforces on the caller-supplied buffer below
// ("The input data is too large. The maximum size is 4294967295."): the byte
// length is carried in an `unsigned` throughout, so uint32 max is the hard
// limit a DOMArrayBufferView here can describe.
constexpr uint64_t kMaxImageDataByteLength =
    std::numeric_limits<uint32_t>::max();

}  // namespace

ImageData* ImageData::ValidateAndCreate(
    unsigned width,
    std::optional<unsigned> height,
    std::optional<NotShared<DOMArrayBufferView>> data,
    const ImageDataSettings* settings,
    ValidateAndCreateParams params,
    ExceptionState& exception_state) {
  gfx::Size size;
  if (params.require_canvas_floating_point &&
      !RuntimeEnabledFeatures::CanvasFloatingPointEnabled()) {
    exception_state.ThrowTypeError("Overload resolution failed.");
    return nullptr;
  }

  if (!width) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kIndexSizeError,
        "The source width is zero or not a number.");
    return nullptr;
  }
  if (width > static_cast<unsigned>(std::numeric_limits<int>::max())) {
    // TODO(crbug.com/1273969): Should throw RangeError instead.
    exception_state.ThrowDOMException(
        DOMExceptionCode::kIndexSizeError,
        "The requested image size exceeds the supported range.");
    return nullptr;
  }
  size.set_width(width);

  if (height) {
    if (!*height) {
      exception_state.ThrowDOMException(
          DOMExceptionCode::kIndexSizeError,
          "The source height is zero or not a number.");
      return nullptr;
    }
    if (height > static_cast<unsigned>(std::numeric_limits<int>::max())) {
      // TODO(crbug.com/1273969): Should throw RangeError instead.
      exception_state.ThrowDOMException(
          DOMExceptionCode::kIndexSizeError,
          "The requested image size exceeds the supported range.");
      return nullptr;
    }
    size.set_height(*height);
  }

  // Ensure the size does not overflow.
  unsigned size_in_elements = 0;
  {
    // Please note that the number "4" in the means number of channels required
    // to describe a pixel, namely, red, green, blue and alpha.
    base::CheckedNumeric<unsigned> size_in_elements_checked = 4;
    size_in_elements_checked *= size.width();
    size_in_elements_checked *= size.height();
    if (!params.context_2d_error_mode) {
      if (!size_in_elements_checked.IsValid()) {
        // TODO(crbug.com/1273969): Should throw RangeError instead.
        exception_state.ThrowDOMException(
            DOMExceptionCode::kIndexSizeError,
            "The requested image size exceeds the supported range.");
        return nullptr;
      }
    }
    if (!size_in_elements_checked.IsValid() ||
        size_in_elements_checked.ValueOrDie() > kMaxImageDataByteLength) {
      exception_state.ThrowRangeError("Out of memory at ImageData creation.");
      return nullptr;
    }
    size_in_elements = size_in_elements_checked.ValueOrDie();
  }

  // Query the color space and storage format from |settings|.
  PredefinedColorSpace color_space = params.default_color_space;
  SkColorType color_type = kRGBA_8888_SkColorType;
  if (settings) {
    if (settings->hasColorSpace() &&
        !ValidateAndConvertColorSpace(settings->colorSpace(), color_space,
                                      exception_state)) {
      return nullptr;
    }
    if (settings->hasPixelFormat()) {
      switch (settings->pixelFormat().AsEnum()) {
        case V8ImageDataPixelFormat::Enum::kRgbaUnorm8:
          color_type = kRGBA_8888_SkColorType;
          break;
        case V8ImageDataPixelFormat::Enum::kRgbaFloat16:
          color_type = kRGBA_F16_SkColorType;
          break;
        case V8ImageDataPixelFormat::Enum::kRgbaFloat32:
          color_type = kRGBA_F32_SkColorType;
          break;
      }
    }
  }

  // If |data| is provided, ensure it is a reasonable format, and that it can
  // work with |size| and |color_type|.
  if (data) {
    DCHECK(data);
    switch ((*data)->GetType()) {
      case DOMArrayBufferView::ViewType::kTypeUint8Clamped:
        if (color_type != kRGBA_8888_SkColorType) {
          exception_state.ThrowDOMException(
              DOMExceptionCode::kInvalidStateError,
              "Uint8ClampedArray must use rgba-unorm8 pixelFormat.");
          return nullptr;
        }
        break;
      case DOMArrayBufferView::ViewType::kTypeFloat16:
        if (color_type != kRGBA_F16_SkColorType) {
          exception_state.ThrowDOMException(
              DOMExceptionCode::kInvalidStateError,
              "Float16Array must use rgba-float16 pixelFormat.");
          return nullptr;
        }
        break;
      case DOMArrayBufferView::ViewType::kTypeFloat32:
        if (color_type != kRGBA_F32_SkColorType) {
          exception_state.ThrowDOMException(
              DOMExceptionCode::kInvalidStateError,
              "Float32Array must use rgba-float32 pixelFormat.");
          return nullptr;
        }
        break;
      default:
        exception_state.ThrowDOMException(
            DOMExceptionCode::kNotSupportedError,
            "The input data type is not supported.");
        return nullptr;
    }
    static_assert(
        std::numeric_limits<unsigned>::max() >=
            std::numeric_limits<uint32_t>::max(),
        "We use UINT32_MAX as the upper bound of the input size and expect "
        "that the result fits into an `unsigned`.");

    unsigned data_length_in_bytes = 0;
    if (!base::CheckedNumeric<uint32_t>((*data)->byteLength())
             .AssignIfValid(&data_length_in_bytes)) {
      exception_state.ThrowDOMException(
          DOMExceptionCode::kNotSupportedError,
          "The input data is too large. The maximum size is 4294967295.");
      return nullptr;
    }
    if (!data_length_in_bytes) {
      exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                        "The input data has zero elements.");
      return nullptr;
    }

    const unsigned data_length_in_elements =
        data_length_in_bytes / (*data)->TypeSize();
    if (data_length_in_elements % 4) {
      exception_state.ThrowDOMException(
          DOMExceptionCode::kInvalidStateError,
          "The input data length is not a multiple of 4.");
      return nullptr;
    }

    const unsigned data_length_in_pixels = data_length_in_elements / 4;
    if (data_length_in_pixels % width) {
      exception_state.ThrowDOMException(
          DOMExceptionCode::kIndexSizeError,
          "The input data length is not a multiple of (4 * width).");
      return nullptr;
    }

    const unsigned expected_height = data_length_in_pixels / width;
    if (height) {
      if (*height != expected_height) {
        exception_state.ThrowDOMException(
            DOMExceptionCode::kIndexSizeError,
            "The input data length is not equal to (4 * width * height).");
        return nullptr;
      }
    } else {
      size.set_height(expected_height);
    }
  }

  NotShared<DOMArrayBufferView> allocated_data;
  if (!data) {
    allocated_data = AllocateAndValidateDataArray(
        size_in_elements, color_type, params.zero_initialize, exception_state);
    if (!allocated_data)
      return nullptr;
  }

  return MakeGarbageCollected<ImageData>(size, data ? *data : allocated_data,
                                         color_space, color_type);
}

NotShared<DOMArrayBufferView> ImageData::AllocateAndValidateDataArray(
    const unsigned& length,
    SkColorType color_type,
    bool zero_initialize,
    ExceptionState& exception_state) {
  if (!length)
    return NotShared<DOMArrayBufferView>();

  NotShared<DOMArrayBufferView> data_array;
  switch (color_type) {
    case kRGBA_8888_SkColorType:
      data_array = NotShared<DOMArrayBufferView>(
          zero_initialize
              ? DOMUint8ClampedArray::CreateOrNull(length)
              : DOMUint8ClampedArray::CreateUninitializedOrNull(length));
      break;
    case kRGBA_F16_SkColorType:
      data_array = NotShared<DOMArrayBufferView>(
          zero_initialize ? DOMFloat16Array::CreateOrNull(length)
                          : DOMFloat16Array::CreateUninitializedOrNull(length));
      break;
    case kRGBA_F32_SkColorType:
      data_array = NotShared<DOMArrayBufferView>(
          zero_initialize ? DOMFloat32Array::CreateOrNull(length)
                          : DOMFloat32Array::CreateUninitializedOrNull(length));
      break;
    default:
      NOTREACHED();
  }

  size_t expected_size;
  if (!data_array || (!base::CheckMul(length, data_array->TypeSize())
                           .AssignIfValid(&expected_size) &&
                      expected_size != data_array->byteLength())) {
    exception_state.ThrowRangeError("Out of memory at ImageData creation");
    return NotShared<DOMArrayBufferView>();
  }

  return data_array;
}

V8PredefinedColorSpace ImageData::colorSpace() const {
  return PredefinedColorSpaceToV8(color_space_);
}

V8ImageDataPixelFormat ImageData::pixelFormat() const {
  switch (color_type_) {
    case kRGBA_8888_SkColorType:
      return V8ImageDataPixelFormat(V8ImageDataPixelFormat::Enum::kRgbaUnorm8);
    case kRGBA_F16_SkColorType:
      return V8ImageDataPixelFormat(V8ImageDataPixelFormat::Enum::kRgbaFloat16);
    case kRGBA_F32_SkColorType:
      return V8ImageDataPixelFormat(V8ImageDataPixelFormat::Enum::kRgbaFloat32);
    default:
      NOTREACHED();
  }
}

bool ImageData::IsBufferBaseDetached() const {
  switch (data_->GetContentType()) {
    case V8ImageDataArray::ContentType::kFloat32Array:
      return data_->GetAsFloat32Array()->BufferBase()->IsDetached();
    case V8ImageDataArray::ContentType::kFloat16Array:
      return data_->GetAsFloat16Array()->BufferBase()->IsDetached();
    case V8ImageDataArray::ContentType::kUint8ClampedArray:
      return data_->GetAsUint8ClampedArray()->BufferBase()->IsDetached();
  }

  NOTREACHED();
}

base::span<uint8_t> ImageData::RawByteSpan() const {
  CHECK(!IsBufferBaseDetached());
  switch (data_->GetContentType()) {
    case V8ImageDataArray::ContentType::kFloat32Array:
      return data_->GetAsFloat32Array()->ByteSpan();
    case V8ImageDataArray::ContentType::kFloat16Array:
      return data_->GetAsFloat16Array()->ByteSpan();
    case V8ImageDataArray::ContentType::kUint8ClampedArray:
      return data_->GetAsUint8ClampedArray()->ByteSpan();
  }

  NOTREACHED();
}

SkPixmap ImageData::GetSkPixmap() const {
  base::span<const uint8_t> data = RawByteSpan();
  SkImageInfo info =
      SkImageInfo::Make(width(), height(), color_type_, kUnpremul_SkAlphaType,
                        PredefinedColorSpaceToSkColorSpace(color_space_));
  return SkPixmap(info, data.data(), info.minRowBytes());
}

void ImageData::Trace(Visitor* visitor) const {
  visitor->Trace(settings_);
  visitor->Trace(data_);
  visitor->Trace(data_u8_);
  visitor->Trace(data_f16_);
  visitor->Trace(data_f32_);
  ScriptWrappable::Trace(visitor);
}

ImageData::ImageData(const gfx::Size& size,
                     NotShared<DOMArrayBufferView> data,
                     PredefinedColorSpace color_space,
                     SkColorType color_type)
    : size_(size),
      settings_(ImageDataSettings::Create()),
      color_space_(color_space),
      color_type_(color_type) {
  DCHECK_GE(size.width(), 0);
  DCHECK_GE(size.height(), 0);
  DCHECK(data);

  data_u8_.Clear();
  data_f16_.Clear();
  data_f32_.Clear();

  if (settings_) {
    settings_->setColorSpace(colorSpace());
    settings_->setPixelFormat(pixelFormat());
  }

  switch (color_type) {
    case kRGBA_8888_SkColorType:
      DCHECK_EQ(data->GetType(),
                DOMArrayBufferView::ViewType::kTypeUint8Clamped);
      data_u8_ = data;
      DCHECK(data_u8_);
      SECURITY_CHECK(
          (base::CheckedNumeric<size_t>(size.width()) * size.height() * 4)
              .ValueOrDie() <= data_u8_->length());
      data_ = MakeGarbageCollected<V8ImageDataArray>(data_u8_);
      break;

    case kRGBA_F16_SkColorType:
      DCHECK_EQ(data->GetType(), DOMArrayBufferView::ViewType::kTypeFloat16);
      data_f16_ = data;
      DCHECK(data_f16_);
      SECURITY_CHECK(
          (base::CheckedNumeric<size_t>(size.width()) * size.height() * 4)
              .ValueOrDie() <= data_f16_->length());
      data_ = MakeGarbageCollected<V8ImageDataArray>(data_f16_);
      break;

    case kRGBA_F32_SkColorType:
      DCHECK_EQ(data->GetType(), DOMArrayBufferView::ViewType::kTypeFloat32);
      data_f32_ = data;
      DCHECK(data_f32_);
      SECURITY_CHECK(
          (base::CheckedNumeric<size_t>(size.width()) * size.height() * 4)
              .ValueOrDie() <= data_f32_->length());
      data_ = MakeGarbageCollected<V8ImageDataArray>(data_f32_);
      break;

    default:
      NOTREACHED();
  }
}

}  // namespace blink
