// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/css/font_face_set.h"

#include "base/task/single_thread_task_runner.h"
#include "third_party/blink/renderer/bindings/core/v8/v8_font_face_set_load_status.h"
#include "third_party/blink/renderer/core/css/font_face_cache.h"
#include "third_party/blink/renderer/core/css/font_face_set_load_event.h"
#include "third_party/blink/renderer/platform/font_family_names.h"
#include "third_party/blink/renderer/platform/fonts/font.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/wtf/functional.h"
#include "third_party/blink/renderer/platform/wtf/text/strcat.h"

namespace blink {

const int FontFaceSet::kDefaultFontSize = 10;

// static
const AtomicString& FontFaceSet::DefaultFontFamily() {
  return font_family_names::kSansSerif;
}

void FontFaceSet::HandlePendingEventsAndPromisesSoon() {
  if (!pending_task_queued_) {
    if (auto* context = GetExecutionContext()) {
      pending_task_queued_ = true;
      context->GetTaskRunner(TaskType::kFontLoading)
          ->PostTask(FROM_HERE,
                     BindOnce(&FontFaceSet::HandlePendingEventsAndPromises,
                              WrapPersistent(this)));
    }
  }
}

void FontFaceSet::HandlePendingEventsAndPromises() {
  pending_task_queued_ = false;
  if (!GetExecutionContext()) {
    return;
  }
  FireLoadingEvent();
  FireDoneEventIfPossible();
}

void FontFaceSet::FireLoadingEvent() {
  if (should_fire_loading_event_) {
    should_fire_loading_event_ = false;
    DispatchEvent(
        *FontFaceSetLoadEvent::CreateForFontFaces(event_type_names::kLoading));
  }
}

V8FontFaceSetLoadStatus FontFaceSet::status() const {
  return V8FontFaceSetLoadStatus(is_loading_
                                     ? V8FontFaceSetLoadStatus::Enum::kLoading
                                     : V8FontFaceSetLoadStatus::Enum::kLoaded);
}

void FontFaceSet::Trace(Visitor* visitor) const {
  visitor->Trace(non_css_connected_faces_);
  visitor->Trace(loading_fonts_);
  visitor->Trace(loaded_fonts_);
  visitor->Trace(failed_fonts_);
  ExecutionContextClient::Trace(visitor);
  EventTarget::Trace(visitor);
  FontFace::LoadFontCallback::Trace(visitor);
}

wtf_size_t FontFaceSet::size() const {
  if (!InActiveContext()) {
    return non_css_connected_faces_.size();
  }
  return CSSConnectedFontFaceList().size() + non_css_connected_faces_.size();
}

void FontFaceSet::AddFontFacesToFontFaceCache(FontFaceCache* font_face_cache) {
  for (const auto& font_face : non_css_connected_faces_) {
    font_face_cache->AddFontFace(font_face, false);
  }
}

void FontFaceSet::AddToLoadingFonts(FontFace* font_face) {
  if (!is_loading_) {
    is_loading_ = true;
    should_fire_loading_event_ = true;
    ready_state_ = ReadyState::kPending;
    HandlePendingEventsAndPromisesSoon();
  }
  loading_fonts_.insert(font_face);
  font_face->AddCallback(this);
}

void FontFaceSet::RemoveFromLoadingFonts(FontFace* font_face) {
  loading_fonts_.erase(font_face);
  if (loading_fonts_.empty()) {
    HandlePendingEventsAndPromisesSoon();
  }
}

bool FontFaceSet::check(const String& font_string,
                        const String& text,
                        ExceptionState& exception_state) {
  if (!InActiveContext()) {
    return false;
  }

  const Font* font = ResolveFontStyle(font_string);
  if (!font) {
    exception_state.ThrowDOMException(
        DOMExceptionCode::kSyntaxError,
        StrCat({"Could not resolve '", font_string, "' as a font."}));
    return false;
  }

  FontSelector* font_selector = GetFontSelector();
  FontFaceCache* font_face_cache = font_selector->GetFontFaceCache();

  unsigned index = 0;
  while (index < text.length()) {
    UChar32 c = text.CodePointAtOrZero(index);
    index += U16_LENGTH(c);

    for (const FontFamily* f = &font->GetFontDescription().Family(); f;
         f = f->Next()) {
      if (f->FamilyIsGeneric() || font_selector->IsPlatformFamilyMatchAvailable(
                                      font->GetFontDescription(), *f)) {
        continue;
      }

      CSSSegmentedFontFace* face =
          font_face_cache->Get(font->GetFontDescription(), f->FamilyName());
      if (face && !face->CheckFont(c)) {
        return false;
      }
    }
  }
  return true;
}

void FontFaceSet::FireDoneEvent() {
  if (is_loading_) {
    FontFaceSetLoadEvent* done_event = nullptr;
    FontFaceSetLoadEvent* error_event = nullptr;
    done_event = FontFaceSetLoadEvent::CreateForFontFaces(
        event_type_names::kLoadingdone, loaded_fonts_);
    loaded_fonts_.clear();
    if (!failed_fonts_.empty()) {
      error_event = FontFaceSetLoadEvent::CreateForFontFaces(
          event_type_names::kLoadingerror, failed_fonts_);
      failed_fonts_.clear();
    }
    is_loading_ = false;
    DispatchEvent(*done_event);
    if (error_event) {
      DispatchEvent(*error_event);
    }
  }

  ready_state_ = ReadyState::kSettled;
}

bool FontFaceSet::ShouldSignalReady() const {
  if (!loading_fonts_.empty()) {
    return false;
  }
  return is_loading_ || ready_state_ == ReadyState::kPending;
}

}  // namespace blink
