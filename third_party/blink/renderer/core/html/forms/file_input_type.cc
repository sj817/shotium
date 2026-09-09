/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011 Apple Inc. All
 * rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "third_party/blink/renderer/core/html/forms/file_input_type.h"

#include "third_party/blink/public/strings/grit/blink_strings.h"
#include "third_party/blink/renderer/core/dom/events/event.h"
#include "third_party/blink/renderer/core/dom/shadow_root.h"
#include "third_party/blink/renderer/core/fileapi/file.h"
#include "third_party/blink/renderer/core/fileapi/file_list.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/core/html/forms/form_controller.h"
#include "third_party/blink/renderer/core/html/forms/form_data.h"
#include "third_party/blink/renderer/core/html/forms/html_input_element.h"
#include "third_party/blink/renderer/core/html/shadow/shadow_element_names.h"
#include "third_party/blink/renderer/core/html_names.h"
#include "third_party/blink/renderer/core/input_type_names.h"
#include "third_party/blink/renderer/core/keywords.h"
#include "third_party/blink/renderer/core/layout/layout_block_flow.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"
#include "third_party/blink/renderer/platform/text/platform_locale.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

FileInputType::FileInputType(HTMLInputElement& element)
    : InputType(Type::kFile, element),
      KeyboardClickableInputTypeView(element),
      file_list_(MakeGarbageCollected<FileList>()) {}

void FileInputType::Trace(Visitor* visitor) const {
  visitor->Trace(file_list_);
  KeyboardClickableInputTypeView::Trace(visitor);
  InputType::Trace(visitor);
}

InputTypeView* FileInputType::CreateView() {
  return this;
}

template <typename ItemType, typename VectorType>
VectorType CreateFilesFrom(const FormControlState& state,
                           ItemType (*factory)(const FormControlState&,
                                               wtf_size_t&)) {
  VectorType files;
  files.ReserveInitialCapacity(state.ValueSize() / 3);
  for (wtf_size_t i = 0; i < state.ValueSize();) {
    files.push_back(factory(state, i));
  }
  return files;
}

template <typename ItemType, typename VectorType>
VectorType CreateFilesFrom(const FormControlState& state,
                           ExecutionContext* execution_context,
                           ItemType (*factory)(ExecutionContext*,
                                               const FormControlState&,
                                               wtf_size_t&)) {
  VectorType files;
  files.ReserveInitialCapacity(state.ValueSize() / 3);
  for (wtf_size_t i = 0; i < state.ValueSize();) {
    files.push_back(factory(execution_context, state, i));
  }
  return files;
}

Vector<String> FileInputType::FilesFromFormControlState(
    const FormControlState& state) {
  return CreateFilesFrom<String, Vector<String>>(state,
                                                 &File::PathFromControlState);
}

FormControlState FileInputType::SaveFormControlState() const {
  if (file_list_->IsEmpty() || GetElement()
                                   .GetDocument()
                                   .EnsureFormController()
                                   .DropReferencedFilePaths()) {
    return FormControlState();
  }
  FormControlState state;
  unsigned num_files = file_list_->length();
  for (unsigned i = 0; i < num_files; ++i)
    file_list_->item(i)->AppendToControlState(state);
  return state;
}

void FileInputType::RestoreFormControlState(const FormControlState& state) {
  if (state.ValueSize() % 3)
    return;
  ExecutionContext* execution_context = GetElement().GetExecutionContext();
  HeapVector<Member<File>> file_vector =
      CreateFilesFrom<File*, HeapVector<Member<File>>>(
          state, execution_context, &File::CreateFromControlState);
  auto* file_list = MakeGarbageCollected<FileList>();
  for (const auto& file : file_vector)
    file_list->Append(file);
  SetFiles(file_list);
}

void FileInputType::AppendToFormData(FormData& form_data) const {
  FileList* file_list = GetElement().files();
  unsigned num_files = file_list->length();
  ExecutionContext* context = GetElement().GetExecutionContext();
  if (num_files == 0) {
    form_data.AppendFromElement(GetElement().GetName(),
                                MakeGarbageCollected<File>(context, ""));
    return;
  }

  for (unsigned i = 0; i < num_files; ++i) {
    form_data.AppendFromElement(GetElement().GetName(), file_list->item(i));
  }
}

bool FileInputType::ValueMissing(const String& value) const {
  return GetElement().IsRequired() && value.empty();
}

String FileInputType::ValueMissingText() const {
  return GetLocale().QueryString(
      GetElement().Multiple() ? IDS_FORM_VALIDATION_VALUE_MISSING_MULTIPLE_FILE
                              : IDS_FORM_VALIDATION_VALUE_MISSING_FILE);
}

void FileInputType::AdjustStyle(ComputedStyleBuilder& builder) {
  builder.SetShouldIgnoreOverflowPropertyForInlineBlockBaseline();
}

LayoutObject* FileInputType::CreateLayoutObject(const ComputedStyle&) const {
  return MakeGarbageCollected<LayoutBlockFlow>(&GetElement());
}

InputType::ValueMode FileInputType::GetValueMode() const {
  return ValueMode::kFilename;
}

bool FileInputType::CanSetStringValue() const {
  return false;
}

FileList* FileInputType::Files() {
  return file_list_.Get();
}

bool FileInputType::CanSetValue(const String& value) {
  // For security reasons, we don't allow setting the filename, but we do allow
  // clearing it.  The HTML5 spec (as of the 10/24/08 working draft) says that
  // the value attribute isn't applicable to the file upload control at all, but
  // for now we are keeping this behavior to avoid breaking existing websites
  // that may be relying on this.
  return value.empty();
}

String FileInputType::ValueInFilenameValueMode() const {
  if (file_list_->IsEmpty())
    return String();

  // HTML5 tells us that we're supposed to use this goofy value for
  // file input controls. Historically, browsers revealed the real
  // file path, but that's a privacy problem. Code on the web
  // decided to try to parse the value by looking for backslashes
  // (because that's what Windows file paths use). To be compatible
  // with that code, we make up a fake path for the file.
  return StrCat({"C:\\fakepath\\", file_list_->item(0)->name()});
}

void FileInputType::SetValue(const String&,
                             bool value_changed,
                             TextFieldEventBehavior,
                             TextControlSetValueSelection) {
  if (!value_changed)
    return;

  file_list_->clear();
  GetElement().SetNeedsValidityCheck();
  UpdateView();
}

void FileInputType::CountUsage() {
  ExecutionContext* context = GetElement().GetExecutionContext();
  if (context->IsSecureContext())
    UseCounter::Count(context, WebFeature::kInputTypeFileSecureOrigin);
  else
    UseCounter::Count(context, WebFeature::kInputTypeFileInsecureOrigin);
}

void FileInputType::CreateShadowSubtree() {
  DCHECK(IsShadowHost(GetElement()));
  Document& document = GetElement().GetDocument();

  auto* button = MakeGarbageCollected<HTMLInputElement>(document);
  button->setType(input_type_names::kButton);
  button->setAttribute(
      html_names::kValueAttr,
      AtomicString(GetLocale().QueryString(
          GetElement().Multiple() ? IDS_FORM_MULTIPLE_FILES_BUTTON_LABEL
                                  : IDS_FORM_FILE_BUTTON_LABEL)));
  button->SetShadowPseudoId(shadow_element_names::kPseudoFileUploadButton);
  button->setAttribute(html_names::kIdAttr,
                       shadow_element_names::kIdFileUploadButton);
  button->SetActive(GetElement().CanReceiveDroppedFiles());
  GetElement().UserAgentShadowRoot()->AppendChild(button);

  auto* span = document.CreateRawElement(html_names::kSpanTag);
  GetElement().UserAgentShadowRoot()->AppendChild(span);

  // The file input element is presented to AX as one node with the role button,
  // instead of the individual button and text nodes. That's the reason we hide
  // the shadow root elements of the file input in the AX tree.
  button->setAttribute(html_names::kAriaHiddenAttr, keywords::kTrue);
  span->setAttribute(html_names::kAriaHiddenAttr, keywords::kTrue);

  UpdateView();
}

HTMLInputElement* FileInputType::UploadButton() const {
  Element* element = GetElement().EnsureShadowSubtree()->getElementById(
      shadow_element_names::kIdFileUploadButton);
  CHECK(!element || IsA<HTMLInputElement>(element));
  return To<HTMLInputElement>(element);
}

Node* FileInputType::FileStatusElement() const {
  return GetElement().EnsureShadowSubtree()->lastChild();
}

void FileInputType::DisabledAttributeChanged(DisabledChangedReason reason) {
  if (Element* button = UploadButton()) {
    button->SetBooleanAttribute(html_names::kDisabledAttr,
                                GetElement().IsDisabledFormControl());
  }
}

void FileInputType::MultipleAttributeChanged() {
  if (Element* button = UploadButton()) {
    button->setAttribute(
        html_names::kValueAttr,
        AtomicString(GetLocale().QueryString(
            GetElement().Multiple() ? IDS_FORM_MULTIPLE_FILES_BUTTON_LABEL
                                    : IDS_FORM_FILE_BUTTON_LABEL)));
  }
}

bool FileInputType::SetFiles(FileList* files) {
  if (!files)
    return false;

  bool files_changed = false;
  if (files->length() != file_list_->length()) {
    files_changed = true;
  } else {
    for (unsigned i = 0; i < files->length(); ++i) {
      if (!files->item(i)->HasSameSource(*file_list_->item(i))) {
        files_changed = true;
        break;
      }
    }
  }

  file_list_ = files;

  GetElement().NotifyFormStateChanged();
  GetElement().SetNeedsValidityCheck();
  UpdateView();
  return files_changed;
}

void FileInputType::SetFilesAndDispatchEvents(FileList* files) {
  if (SetFiles(files)) {
    // This call may cause destruction of this instance.
    // input instance is safe since it is ref-counted.
    GetElement().DispatchInputEvent();
    GetElement().DispatchChangeEvent();
    // Used to notify AXObjectCache that the value changed. AXObjectCache is
    // gone (no accessibility tree in a screenshot renderer).
  } else {
    GetElement().DispatchCancelEvent();
  }
}

String FileInputType::DefaultToolTip(const InputTypeView&) const {
  FileList* file_list = file_list_.Get();
  unsigned list_size = file_list->length();
  if (!list_size) {
    return GetLocale().QueryString(IDS_FORM_FILE_NO_FILE_LABEL);
  }

  StringBuilder names;
  for (wtf_size_t i = 0; i < list_size; ++i) {
    names.Append(file_list->item(i)->name());
    if (i != list_size - 1)
      names.Append('\n');
  }
  return names.ToString();
}

void FileInputType::CopyNonAttributeProperties(const HTMLInputElement& source) {
  DCHECK(file_list_->IsEmpty());
  const FileList* source_list = source.files();
  for (unsigned i = 0; i < source_list->length(); ++i)
    file_list_->Append(source_list->item(i)->Clone());
}

String FileInputType::FileStatusText() const {
  Locale& locale = GetLocale();

  if (file_list_->IsEmpty())
    return locale.QueryString(IDS_FORM_FILE_NO_FILE_LABEL);

  if (file_list_->length() == 1)
    return LayoutTheme::GetTheme().DisplayNameForFile(*file_list_->item(0));

  return locale.QueryString(
      IDS_FORM_FILE_MULTIPLE_UPLOAD,
      locale.ConvertToLocalizedNumber(String::Number(file_list_->length())));
}

void FileInputType::UpdateView() {
  if (auto* span = FileStatusElement())
    span->setTextContent(FileStatusText());
}

}  // namespace blink
