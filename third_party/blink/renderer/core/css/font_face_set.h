// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_FONT_FACE_SET_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_FONT_FACE_SET_H_

#include "third_party/blink/public/platform/task_type.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/font_face.h"
#include "third_party/blink/renderer/core/dom/events/event_listener.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/event_target_names.h"
#include "third_party/blink/renderer/core/execution_context/execution_context_lifecycle_observer.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/fonts/font_selector.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_hash_set.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_linked_hash_set.h"

namespace blink {

class Font;
class FontFaceCache;
class V8FontFaceSetLoadStatus;

class CORE_EXPORT FontFaceSet : public EventTarget,
                                public ExecutionContextClient,
                                public FontFace::LoadFontCallback {
  DEFINE_WRAPPERTYPEINFO();

 public:
  explicit FontFaceSet(ExecutionContext& context)
      : ExecutionContextClient(&context) {}
  FontFaceSet(const FontFaceSet&) = delete;
  FontFaceSet& operator=(const FontFaceSet&) = delete;
  ~FontFaceSet() override = default;

  DEFINE_ATTRIBUTE_EVENT_LISTENER(loading, kLoading)
  DEFINE_ATTRIBUTE_EVENT_LISTENER(loadingdone, kLoadingdone)
  DEFINE_ATTRIBUTE_EVENT_LISTENER(loadingerror, kLoadingerror)

  bool check(const String& font, const String& text, ExceptionState&);

  ExecutionContext* GetExecutionContext() const override {
    return ExecutionContextClient::GetExecutionContext();
  }

  const AtomicString& InterfaceName() const override {
    return event_target_names::kFontFaceSet;
  }

  void AddFontFacesToFontFaceCache(FontFaceCache*);

  wtf_size_t size() const;
  V8FontFaceSetLoadStatus status() const;

  void Trace(Visitor*) const override;

 protected:
  static const int kDefaultFontSize;
  static const AtomicString& DefaultFontFamily();

  virtual const Font* ResolveFontStyle(const String&) = 0;
  virtual bool InActiveContext() const = 0;
  virtual FontSelector* GetFontSelector() const = 0;
  virtual const HeapLinkedHashSet<Member<FontFace>>& CSSConnectedFontFaceList()
      const = 0;
  bool IsCSSConnectedFontFace(FontFace* font_face) const {
    return CSSConnectedFontFaceList().Contains(font_face);
  }

  virtual void FireDoneEventIfPossible() = 0;

  void AddToLoadingFonts(FontFace*);
  void RemoveFromLoadingFonts(FontFace*);
  void HandlePendingEventsAndPromisesSoon();
  bool ShouldSignalReady() const;
  void FireDoneEvent();

  // Tracks the state that used to be held by the `ready` promise: the set
  // starts out "not yet settled", latches to settled once every pending font
  // has finished loading, and is reset back to pending whenever a new font
  // starts loading. `FireDoneEventIfPossible()` is gated on this latch, so it
  // must keep its exact transitions even though nothing observes a promise.
  enum class ReadyState { kPending, kSettled };

  bool is_loading_ = false;
  bool should_fire_loading_event_ = false;
  bool pending_task_queued_ = false;
  HeapLinkedHashSet<Member<FontFace>> non_css_connected_faces_;
  HeapHashSet<Member<FontFace>> loading_fonts_;
  FontFaceArray loaded_fonts_;
  FontFaceArray failed_fonts_;
  ReadyState ready_state_ = ReadyState::kPending;

 private:
  void HandlePendingEventsAndPromises();
  void FireLoadingEvent();
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_FONT_FACE_SET_H_
