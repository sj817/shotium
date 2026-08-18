/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/fileapi/blob.h"

#include <limits>
#include <memory>
#include <utility>

#include "base/check.h"
#include "base/notreached.h"
#include "base/task/single_thread_task_runner.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_blob_property_bag.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_union_arraybuffer_arraybufferview_blob_usvstring.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/fileapi/file_read_type.h"
#include "third_party/blink/renderer/core/fileapi/file_reader_client.h"
#include "third_party/blink/renderer/core/fileapi/file_reader_loader.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer.h"
#include "third_party/blink/renderer/core/typed_arrays/dom_array_buffer_view.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/self_keep_alive.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/wtf/text/text_encoding.h"

namespace blink {

namespace {

// http://dev.w3.org/2006/webapi/FileAPI/#constructorBlob
bool IsValidBlobType(const String& type) {
  for (unsigned i = 0; i < type.length(); ++i) {
    UChar c = type[i];
    if (c < 0x20 || c > 0x7E) {
      return false;
    }
  }
  return true;
}

}  // namespace

Blob::Blob(scoped_refptr<BlobDataHandle> data_handle)
    : blob_data_handle_(std::move(data_handle)) {}

Blob::~Blob() = default;

// static
Blob* Blob::Create(ExecutionContext* context,
                   const HeapVector<Member<V8BlobPart>>& blob_parts,
                   const BlobPropertyBag* options) {
  DCHECK(options->hasType());
  DCHECK(options->hasEndings());
  const bool normalize_line_endings_to_native =
      (options->endings() == V8EndingType::Enum::kNative);
  if (normalize_line_endings_to_native)
    UseCounter::Count(context, WebFeature::kFileAPINativeLineEndings);
  UseCounter::Count(context, WebFeature::kCreateObjectBlob);

  auto blob_data = std::make_unique<BlobData>();
  blob_data->SetContentType(NormalizeType(options->type()));

  PopulateBlobData(blob_data.get(), blob_parts,
                   normalize_line_endings_to_native);

  uint64_t blob_size = blob_data->length();
  return MakeGarbageCollected<Blob>(
      BlobDataHandle::Create(std::move(blob_data), blob_size));
}

Blob* Blob::Create(base::span<const uint8_t> data, const String& content_type) {
  auto blob_data = std::make_unique<BlobData>();
  blob_data->SetContentType(content_type);
  blob_data->AppendBytes(data);
  uint64_t blob_size = blob_data->length();

  return MakeGarbageCollected<Blob>(
      BlobDataHandle::Create(std::move(blob_data), blob_size));
}

// static
void Blob::PopulateBlobData(BlobData* blob_data,
                            const HeapVector<Member<V8BlobPart>>& parts,
                            bool normalize_line_endings_to_native) {
  for (const auto& item : parts) {
    switch (item->GetContentType()) {
      case V8BlobPart::ContentType::kArrayBuffer: {
        DOMArrayBuffer* array_buffer = item->GetAsArrayBuffer();
        blob_data->AppendBytes(array_buffer->ByteSpan());
        break;
      }
      case V8BlobPart::ContentType::kArrayBufferView: {
        auto&& array_buffer_view = item->GetAsArrayBufferView();
        blob_data->AppendBytes(array_buffer_view->ByteSpan());
        break;
      }
      case V8BlobPart::ContentType::kBlob: {
        item->GetAsBlob()->AppendTo(*blob_data);
        break;
      }
      case V8BlobPart::ContentType::kUSVString: {
        blob_data->AppendText(item->GetAsUSVString(),
                              normalize_line_endings_to_native);
        break;
      }
    }
  }
}

// static
void Blob::ClampSliceOffsets(uint64_t size, int64_t& start, int64_t& end) {
  DCHECK_NE(size, std::numeric_limits<uint64_t>::max());

  // Convert the negative value that is used to select from the end.
  if (start < 0)
    start = start + size;
  if (end < 0)
    end = end + size;

  // Clamp the range if it exceeds the size limit.
  if (start < 0)
    start = 0;
  if (end < 0)
    end = 0;
  if (static_cast<uint64_t>(start) >= size) {
    start = 0;
    end = 0;
  } else if (end < start) {
    end = start;
  } else if (static_cast<uint64_t>(end) > size) {
    end = size;
  }
}

Blob* Blob::slice(int64_t start,
                  int64_t end,
                  const String& content_type,
                  ExceptionState& exception_state) const {
  uint64_t size = this->size();
  ClampSliceOffsets(size, start, end);

  uint64_t length = end - start;
  auto blob_data = std::make_unique<BlobData>();
  blob_data->SetContentType(NormalizeType(content_type));
  blob_data->AppendBlob(blob_data_handle_, start, length);
  return MakeGarbageCollected<Blob>(
      BlobDataHandle::Create(std::move(blob_data), length));
}

scoped_refptr<BlobDataHandle> Blob::GetBlobDataHandleWithKnownSize() const {
  if (!blob_data_handle_->IsSingleUnknownSizeFile()) {
    return blob_data_handle_;
  }
  return BlobDataHandle::Create(blob_data_handle_->Uuid(),
                                blob_data_handle_->GetType(), size(),
                                blob_data_handle_->CloneBlobRemote());
}

void Blob::AppendTo(BlobData& blob_data) const {
  blob_data.AppendBlob(blob_data_handle_, 0, size());
}

void Blob::CloneMojoBlob(mojo::PendingReceiver<mojom::blink::Blob> receiver) {
  blob_data_handle_->CloneBlobRemote(std::move(receiver));
}

mojo::PendingRemote<mojom::blink::Blob> Blob::AsMojoBlob() const {
  return blob_data_handle_->CloneBlobRemote();
}

// static
String Blob::NormalizeType(const String& type) {
  if (type.IsNull()) {
    return g_empty_string;
  }
  if (type.length() > 65535) {
    return g_empty_string;
  }
  if (!IsValidBlobType(type)) {
    return g_empty_string;
  }
  return type.ToAsciiLower();
}

}  // namespace blink
