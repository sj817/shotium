// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HIGHLIGHT_HIGHLIGHT_REGISTRY_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HIGHLIGHT_HIGHLIGHT_REGISTRY_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/live_collection_iterator.h"
#include "third_party/blink/renderer/core/highlight/highlight.h"
#include "third_party/blink/renderer/core/highlight/highlight_registry_map_entry.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/supplementable.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/atomic_string_hash.h"

namespace blink {

// Using LinkedHashSet<HighlightRegistryMapEntry> to store the map entries
// because order of insertion needs to be preserved (for iteration and breaking
// priority ties during painting) and there's no generic LinkedHashMap. Note
// that the hash functions for HighlightRegistryMapEntry don't allow storing
// more than one entry with the same key (highlight name).
using HighlightRegistryMap =
    HeapLinkedHashSet<Member<HighlightRegistryMapEntry>>;

class Element;
class HighlightHitResult;
class HighlightsFromPointOptions;
class LocalDOMWindow;
class LocalFrame;
class Text;

class CORE_EXPORT HighlightRegistry : public ScriptWrappable,
                                      public Supplement<LocalDOMWindow> {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static const char kSupplementName[];
  static HighlightRegistry* From(LocalDOMWindow&);

  explicit HighlightRegistry(LocalDOMWindow&);
  ~HighlightRegistry() override;

  void Trace(blink::Visitor*) const override;

  static HighlightRegistry* GetHighlightRegistry(const Node* node);

  void SetForTesting(AtomicString, Highlight*);
  void RemoveForTesting(AtomicString, Highlight*);
  wtf_size_t size() const { return highlights_.size(); }

  const HighlightRegistryMap& GetHighlights() const { return highlights_; }
  const HashSet<AtomicString>& GetActiveHighlights(const Text& node) const;
  // Returns the active custom highlight names for a replaced element (e.g.
  // <img>), or a reference to a static empty set if none.
  const HashSet<AtomicString>& GetActiveHighlightsForReplacedElement(
      const Element& element) const;
  void ValidateHighlightMarkers();
  void ScheduleRepaint();

  bool GetForceMarkersValidationForTesting() const {
    return force_markers_validation_;
  }

  enum OverlayStackingPosition {
    kOverlayStackingPositionBelow = -1,
    kOverlayStackingPositionEquivalent = 0,
    kOverlayStackingPositionAbove = 1,
  };

  // Compares Highlights by priority and breaks ties by order of insertion to
  // the registry: a higher priority takes precedence, and in the case
  // priorities are the same, the most recently registered Highlight takes
  // precedence.
  int8_t CompareOverlayStackingPosition(
      const AtomicString& highlight_name1,
      const AtomicString& highlight_name2) const;

  HeapVector<Member<HighlightHitResult>> highlightsFromPoint(
      float x,
      float y,
      const HighlightsFromPointOptions* options);

 private:
  bool IsAbstractRangePaintable(AbstractRange*, Document*) const;

  // Adds `highlight_name` to the set of custom highlights tracked as
  // covering the given replaced element.
  void TrackReplacedElementForHighlight(const Element& element,
                                        const AtomicString& highlight_name);

  HighlightRegistryMap highlights_;
  // Only valid after ValidateHighlightMarkers(), used to optimize painting.
  HeapHashMap<WeakMember<const Text>, HashSet<AtomicString>>
      active_highlights_in_node_;
  // Replaced elements (e.g. <img>) covered by custom highlight ranges,
  // tracked here so ReplacedPainter::PaintCustomHighlights can look up the
  // active highlight names at paint time.
  HeapHashMap<WeakMember<const Element>, HashSet<AtomicString>>
      active_highlights_in_replaced_element_;
  uint64_t dom_tree_version_for_validate_highlight_markers_ = 0;
  uint64_t style_version_for_validate_highlight_markers_ = 0;
  bool force_markers_validation_ = true;
  // Number of Highlights registered so far during the lifetime of this
  // HighlightRegistry. Used to store this information for every Highlight
  // registered in order to break ties when determining Highlight precedence.
  uint64_t highlights_registered_ = 0;

  HighlightRegistryMap::iterator GetMapIterator(const AtomicString& key) const {
    return highlights_.Find<HighlightRegistryMapEntryNameTranslator>(key);
  }

  // The frame of the window this registry supplements, or null once that
  // window has been detached from its frame. A registry can be created for an
  // already detached window, because script can reach the CSS.highlights of a
  // destroyed document through a saved reference to its CSS namespace object.
  // Such a registry has nothing to paint or hit test.
  LocalFrame* GetFrame() const;
  Document* GetDocument() const;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HIGHLIGHT_HIGHLIGHT_REGISTRY_H_
