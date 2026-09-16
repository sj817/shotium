/*
 * Copyright (C) 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "third_party/blink/renderer/core/clipboard/data_transfer.h"

#include <memory>
#include <optional>

#include "build/build_config.h"
#include "third_party/blink/renderer/core/clipboard/clipboard_utilities.h"
#include "third_party/blink/renderer/core/clipboard/data_object.h"
#include "third_party/blink/renderer/core/clipboard/data_transfer_access_policy.h"
#include "third_party/blink/renderer/core/clipboard/data_transfer_item.h"
#include "third_party/blink/renderer/core/clipboard/data_transfer_item_list.h"
#include "third_party/blink/renderer/core/fileapi/file_list.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/html/html_image_element.h"
#include "third_party/blink/renderer/core/keywords.h"
#include "third_party/blink/renderer/core/loader/resource/image_resource_content.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string.h"
#include "ui/base/dragdrop/mojom/drag_drop_types.mojom-blink.h"

namespace blink {

namespace {

std::optional<DragOperationsMask> ConvertEffectAllowedToDragOperationsMask(
    const AtomicString& op) {
  // Values specified in
  // https://html.spec.whatwg.org/multipage/dnd.html#dom-datatransfer-effectallowed
  if (op == keywords::kUninitialized) {
    return kDragOperationEvery;
  }
  if (op == keywords::kNone)
    return kDragOperationNone;
  if (op == "copy")
    return kDragOperationCopy;
  if (op == "link")
    return kDragOperationLink;
  if (op == "move")
    return kDragOperationMove;
  if (op == "copyLink") {
    return static_cast<DragOperationsMask>(kDragOperationCopy |
                                           kDragOperationLink);
  }
  if (op == "copyMove") {
    return static_cast<DragOperationsMask>(kDragOperationCopy |
                                           kDragOperationMove);
  }
  if (op == "linkMove") {
    return static_cast<DragOperationsMask>(kDragOperationLink |
                                           kDragOperationMove);
  }
  if (op == "all")
    return kDragOperationEvery;
  return std::nullopt;
}

AtomicString ConvertEffectAllowedToDropEffect(
    const AtomicString& effect_allowed) {
  auto mask = ConvertEffectAllowedToDragOperationsMask(effect_allowed);
  if (!mask.has_value()) {
    return keywords::kNone;
  }
  // The spec [1] doesn't define a specific order drop effects should be
  // prioritized in, and leaves it up to user agents to adapt to their
  // platform's convention.
  // [1] https://html.spec.whatwg.org/multipage/dnd.html#the-dragevent-interface
  // For example, if `effectAllowed` is "all", the entry in the spec table
  // mentions that `dropEffect` should be:
  // > "copy", or, if appropriate, either "link" or "move"
  // In desktop platforms, the usual expectation is that when you drag something
  // within the file system it will be moved (if it's on the same disk).
  // This ordering matches `DragController::DefaultOperationForDrag`.
  if (mask == kDragOperationEvery) {
    return AtomicString("copy");
  }
  if (mask.value() & kDragOperationMove) {
    return AtomicString("move");
  }
  if ((mask.value() & kDragOperationCopy)) {
    return AtomicString("copy");
  }
  if (mask.value() & kDragOperationLink) {
    return AtomicString("link");
  }
  return keywords::kNone;
}

AtomicString ConvertDragOperationsMaskToEffectAllowed(DragOperationsMask op) {
  if (((op & kDragOperationMove) && (op & kDragOperationCopy) &&
       (op & kDragOperationLink)) ||
      (op == kDragOperationEvery))
    return AtomicString("all");
  if ((op & kDragOperationMove) && (op & kDragOperationCopy))
    return AtomicString("copyMove");
  if ((op & kDragOperationMove) && (op & kDragOperationLink))
    return AtomicString("linkMove");
  if ((op & kDragOperationCopy) && (op & kDragOperationLink))
    return AtomicString("copyLink");
  if (op & kDragOperationMove)
    return AtomicString("move");
  if (op & kDragOperationCopy)
    return AtomicString("copy");
  if (op & kDragOperationLink)
    return AtomicString("link");
  return keywords::kNone;
}

// We provide the IE clipboard types (URL and Text), and the clipboard types
// specified in the HTML spec. See
// https://html.spec.whatwg.org/multipage/dnd.html#the-datatransfer-interface
String NormalizeType(const String& type, bool* convert_to_url = nullptr) {
  constexpr char kTypeText[] = "text";
  constexpr char kTypeUrl[] = "url";
  constexpr char kMimeTypePlainTextEtc[] = "text/plain;";

  String clean_type = type.StripWhiteSpace().ToAsciiLower();
  if (clean_type == kTypeText ||
      clean_type.starts_with(kMimeTypePlainTextEtc)) {
    return ui::kMimeTypePlainText;
  }
  if (clean_type == kTypeUrl) {
    if (convert_to_url) {
      *convert_to_url = true;
    }
    return ui::kMimeTypeUriList;
  }
  return clean_type;
}

}  // namespace

// static
DataTransfer* DataTransfer::Create() {
  DataTransfer* data = Create(
      kCopyAndPaste, DataTransferAccessPolicy::kWritable, DataObject::Create());
  data->drop_effect_ = keywords::kNone;
  data->effect_allowed_ = keywords::kNone;
  return data;
}

// static
DataTransfer* DataTransfer::Create(DataTransferType type,
                                   DataTransferAccessPolicy policy,
                                   DataObject* data_object) {
  return MakeGarbageCollected<DataTransfer>(type, policy, data_object);
}

DataTransfer::~DataTransfer() = default;

void DataTransfer::resetDropEffect() {
  drop_effect_ = AtomicString();
}

void DataTransfer::setDropEffect(const AtomicString& effect) {
  if (!IsForDragAndDrop())
    return;

  // The attribute must ignore any attempts to set it to a value other than
  // none, copy, link, and move.
  if (effect != keywords::kNone && effect != "copy" && effect != "link" &&
      effect != "move")
    return;

  // The specification states that dropEffect can be changed at all times, even
  // if the DataTransfer instance is protected or neutered.
  drop_effect_ = effect;
}

void DataTransfer::setEffectAllowed(const AtomicString& effect) {
  if (!IsForDragAndDrop())
    return;

  if (!ConvertEffectAllowedToDragOperationsMask(effect)) {
    // This means that there was no conversion, and the effectAllowed that
    // we are passed isn't a valid effectAllowed, so we should ignore it,
    // and not set |effect_allowed_|.

    // The attribute must ignore any attempts to set it to a value other than
    // none, copy, copyLink, copyMove, link, linkMove, move, all, and
    // uninitialized.
    return;
  }

  if (CanWriteData()) {
    effect_allowed_ = effect;
    data_object_->SetSourceEffectAllowed(effect);
  }
}

void DataTransfer::clearData(const String& type) {
  if (!CanWriteData()) {
    return;
  }
  if (type.IsNull()) {
    // As per spec
    // https://html.spec.whatwg.org/multipage/dnd.html#dom-datatransfer-cleardata,
    // `clearData()` doesn't remove `kFileKind` objects from `item_list_`.
    data_object_->ClearStringItems();
  } else {
    data_object_->ClearData(NormalizeType(type));
  }
}

String DataTransfer::getData(const String& type) const {
  if (!CanReadData())
    return String();

  bool convert_to_url = false;
  String data = data_object_->GetData(NormalizeType(type, &convert_to_url));
  if (!convert_to_url)
    return data;
  return ConvertURIListToURL(data);
}

void DataTransfer::setData(const String& type, const String& data) {
  if (!CanWriteData())
    return;

  data_object_->SetData(NormalizeType(type), data);
}

bool DataTransfer::hasDataStoreItemListChanged() const {
  return data_store_item_list_changed_ || !CanReadTypes();
}

void DataTransfer::OnItemListChanged() {
  data_store_item_list_changed_ = true;
  files_->clear();

  if (!CanReadData()) {
    return;
  }

  for (uint32_t i = 0; i < data_object_->length(); ++i) {
    if (data_object_->Item(i)->Kind() == DataObjectItem::kFileKind) {
      File* file = data_object_->Item(i)->GetAsFile();
      if (file) {
        files_->Append(file);
      }
    }
  }
}

Vector<String> DataTransfer::types() {
  if (!CanReadTypes())
    return Vector<String>();

  data_store_item_list_changed_ = false;
  return data_object_->Types();
}

FileList* DataTransfer::files() const {
  if (!CanReadData()) {
    files_->clear();
    return files_.Get();
  }
  return files_.Get();
}

void DataTransfer::setDragImage(Element* image, int x, int y) {
  DCHECK(image);

  if (!IsForDragAndDrop())
    return;

  // Convert `drag_loc_` from CSS px to physical pixels.
  // `LocalFrame::LayoutZoomFactor` converts from CSS px to physical px by
  // taking into account both device scale factor and page zoom.
  LocalFrame* frame = image->GetDocument().GetFrame();
  gfx::Point location =
      gfx::ScaleToRoundedPoint(gfx::Point(x, y), frame->LayoutZoomFactor());

  auto* html_image_element = DynamicTo<HTMLImageElement>(image);
  if (html_image_element && !image->isConnected())
    SetDragImageResource(html_image_element->CachedImage(), location);
  else
    SetDragImageElement(image, location);
}

void DataTransfer::ClearDragImage() {
  setDragImage(nullptr, nullptr, gfx::Point());
}

void DataTransfer::SetDragImageResource(ImageResourceContent* img,
                                        const gfx::Point& loc) {
  setDragImage(img, nullptr, loc);
}

void DataTransfer::SetDragImageElement(Node* node, const gfx::Point& loc) {
  setDragImage(nullptr, node, loc);
}

void DataTransfer::SetAccessPolicy(DataTransferAccessPolicy policy) {
  // once you go numb, can never go back
  DCHECK(policy_ != DataTransferAccessPolicy::kNumb ||
         policy == DataTransferAccessPolicy::kNumb);
  policy_ = policy;
}

bool DataTransfer::CanReadTypes() const {
  return policy_ == DataTransferAccessPolicy::kReadable ||
         policy_ == DataTransferAccessPolicy::kTypesReadable ||
         policy_ == DataTransferAccessPolicy::kWritable;
}

bool DataTransfer::CanReadData() const {
  return policy_ == DataTransferAccessPolicy::kReadable ||
         policy_ == DataTransferAccessPolicy::kWritable;
}

bool DataTransfer::CanWriteData() const {
  return policy_ == DataTransferAccessPolicy::kWritable;
}

bool DataTransfer::CanSetDragImage() const {
  return policy_ == DataTransferAccessPolicy::kWritable;
}

DragOperationsMask DataTransfer::SourceOperation() const {
  std::optional<DragOperationsMask> op =
      ConvertEffectAllowedToDragOperationsMask(effect_allowed_);
  DCHECK(op);
  return *op;
}

ui::mojom::blink::DragOperation DataTransfer::DestinationOperation() const {
  DCHECK(DropEffectIsInitialized());
  std::optional<DragOperationsMask> op =
      ConvertEffectAllowedToDragOperationsMask(drop_effect_);
  return static_cast<ui::mojom::blink::DragOperation>(*op);
}

void DataTransfer::SetSourceEffectAllowed(const AtomicString& effect) {
  if (!ConvertEffectAllowedToDragOperationsMask(effect)) {
    return;
  }
  effect_allowed_ = effect;
  data_object_->SetSourceEffectAllowed(effect);
}

void DataTransfer::SetSourceOperation(DragOperationsMask op) {
  effect_allowed_ = ConvertDragOperationsMaskToEffectAllowed(op);
}

void DataTransfer::SetDestinationOperationFromEffectAllowed() {
  setDropEffect(ConvertEffectAllowedToDropEffect(effect_allowed_));
}

void DataTransfer::SetDestinationOperation(ui::mojom::blink::DragOperation op) {
  setDropEffect(ConvertDragOperationsMaskToEffectAllowed(
      static_cast<DragOperationsMask>(op)));
}

DataTransferItemList* DataTransfer::items() {
  // TODO(crbug.com/331320416): According to the spec, we are supposed to
  // return the same collection of items each time. We now return a wrapper
  // that always wraps the *same* set of items, so JS shouldn't be able to
  // tell, but we probably still want to fix this.
  return MakeGarbageCollected<DataTransferItemList>(this, data_object_);
}

DataObject* DataTransfer::GetDataObject() const {
  return data_object_.Get();
}

DataTransfer::DataTransfer(DataTransferType type,
                           DataTransferAccessPolicy policy,
                           DataObject* data_object)
    : policy_(policy),
      // A new drag data store starts with effectAllowed "uninitialized".
      // https://html.spec.whatwg.org/multipage/dnd.html#the-drag-data-store
      effect_allowed_(keywords::kUninitialized),
      transfer_type_(type),
      data_object_(data_object),
      data_store_item_list_changed_(true),
      files_(MakeGarbageCollected<FileList>()) {
  data_object_->AddObserver(this);
  OnItemListChanged();
}

void DataTransfer::setDragImage(ImageResourceContent* image,
                                Node* node,
                                const gfx::Point& loc) {
  if (!CanSetDragImage())
    return;

  drag_image_ = image;
  drag_loc_ = loc;
  drag_image_element_ = node;
}

void DataTransfer::Trace(Visitor* visitor) const {
  visitor->Trace(data_object_);
  visitor->Trace(drag_image_);
  visitor->Trace(drag_image_element_);
  visitor->Trace(files_);
  ScriptWrappable::Trace(visitor);
}

}  // namespace blink
