/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "third_party/blink/renderer/core/clipboard/data_object_item.h"

#include "base/time/time.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "third_party/blink/public/mojom/file_system_access/file_system_access_data_transfer_token.mojom-blink.h"
#include "third_party/blink/public/platform/platform.h"
#include "third_party/blink/renderer/core/fileapi/blob.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/image-encoders/image_encoder.h"
#include "third_party/blink/renderer/platform/network/mime/mime_type_registry.h"
#include "ui/base/clipboard/clipboard_constants.h"

namespace blink {

// static
DataObjectItem* DataObjectItem::CreateFromString(const String& type,
                                                 const String& data) {
  DataObjectItem* item =
      MakeGarbageCollected<DataObjectItem>(kStringKind, type);
  item->data_ = data;
  return item;
}

// static
DataObjectItem* DataObjectItem::CreateFromFile(File* file) {
  DataObjectItem* item =
      MakeGarbageCollected<DataObjectItem>(kFileKind, file->type());
  item->file_ = file;
  return item;
}

// static
DataObjectItem* DataObjectItem::CreateFromFileWithFileSystemId(
    File* file,
    const String& file_system_id,
    scoped_refptr<FileSystemAccessDropData> file_system_access_entry) {
  DataObjectItem* item =
      MakeGarbageCollected<DataObjectItem>(kFileKind, file->type());
  item->file_ = file;
  item->file_system_id_ = file_system_id;
  item->file_system_access_entry_ = file_system_access_entry;
  return item;
}

// static
DataObjectItem* DataObjectItem::CreateFromURL(const String& url,
                                              const String& title) {
  DataObjectItem* item =
      MakeGarbageCollected<DataObjectItem>(kStringKind, ui::kMimeTypeUriList);
  item->data_ = url;
  item->title_ = title;
  return item;
}

// static
DataObjectItem* DataObjectItem::CreateFromHTML(const String& html,
                                               const KURL& base_url) {
  DataObjectItem* item =
      MakeGarbageCollected<DataObjectItem>(kStringKind, ui::kMimeTypeHtml);
  item->data_ = html;
  item->base_url_ = base_url;
  return item;
}

// static
DataObjectItem* DataObjectItem::CreateFromFileSharedBuffer(
    scoped_refptr<SharedBuffer> buffer,
    bool is_image_accessible,
    const KURL& source_url,
    const String& filename_extension,
    const AtomicString& content_disposition) {
  DataObjectItem* item = MakeGarbageCollected<DataObjectItem>(
      kFileKind,
      MIMETypeRegistry::GetWellKnownMIMETypeForExtension(filename_extension));
  item->shared_buffer_ = std::move(buffer);
  item->is_image_accessible_ = is_image_accessible;
  item->filename_extension_ = filename_extension;
  item->title_ = content_disposition;
  item->base_url_ = source_url;
  return item;
}

DataObjectItem::DataObjectItem(ItemKind kind, const String& type)
    : kind_(kind), type_(type) {}

File* DataObjectItem::GetAsFile() const {
  if (Kind() != kFileKind)
    return nullptr;

  if (file_)
    return file_.Get();

  // If this file is not backed by |file_| then it must be a |shared_buffer_|.
  DCHECK(shared_buffer_);
  // If dragged image is cross-origin, do not allow access to it.
  if (!is_image_accessible_)
    return nullptr;
  auto data = std::make_unique<BlobData>();
  data->SetContentType(type_);
  for (const auto& span : *shared_buffer_)
    data->AppendBytes(base::as_bytes(span));
  const uint64_t length = data->length();
  auto blob = BlobDataHandle::Create(std::move(data), length);
  return MakeGarbageCollected<File>(
      DecodeUrlEscapeSequences(base_url_.LastPathComponent(),
                               DecodeUrlMode::kUtf8OrIsomorphic),
      base::Time::Now(), std::move(blob));
}

String DataObjectItem::GetAsString() const {
  DCHECK_EQ(kind_, kStringKind);
  return data_;
}

bool DataObjectItem::IsFilename() const {
  return kind_ == kFileKind && file_;
}

bool DataObjectItem::HasFileSystemId() const {
  return kind_ == kFileKind && !file_system_id_.empty();
}

String DataObjectItem::FileSystemId() const {
  return file_system_id_;
}

bool DataObjectItem::HasFileSystemAccessEntry() const {
  return static_cast<bool>(file_system_access_entry_);
}

mojo::PendingRemote<mojom::blink::FileSystemAccessDataTransferToken>
DataObjectItem::CloneFileSystemAccessEntryToken() const {
  DCHECK(HasFileSystemAccessEntry());
  mojo::Remote<mojom::blink::FileSystemAccessDataTransferToken> token_cloner(
      std::move(file_system_access_entry_->data));
  mojo::PendingRemote<mojom::blink::FileSystemAccessDataTransferToken>
      token_clone;
  token_cloner->Clone(token_clone.InitWithNewPipeAndPassReceiver());
  file_system_access_entry_->data = token_cloner.Unbind();
  return token_clone;
}

void DataObjectItem::Trace(Visitor* visitor) const {
  visitor->Trace(file_);
}

}  // namespace blink
