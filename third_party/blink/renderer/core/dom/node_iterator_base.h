/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * Copyright (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2008 Apple Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_ITERATOR_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_ITERATOR_BASE_H_

#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"

namespace blink {

class ExceptionState;
class Node;

class NodeIteratorBase : public GarbageCollectedMixin {
 public:
  // The NodeFilter return values, from the DOM standard. They lived on the
  // generated V8NodeFilter class because that is where the bindings generator
  // puts an interface's constants; the values themselves are spec, not
  // bindings.
  enum : unsigned {
    kFilterAccept = 1,
    kFilterReject = 2,
    kFilterSkip = 3,
  };

  virtual ~NodeIteratorBase() = default;

  Node* root() const { return root_.Get(); }
  unsigned whatToShow() const { return what_to_show_; }

  void Trace(Visitor*) const override;

 protected:
  // The script NodeFilter argument is gone: it is a callback interface, and
  // nothing can supply one. A traversal created without a filter is a case the
  // spec algorithm already handles -- step 4 returns FILTER_ACCEPT -- so this
  // is the same traversal, not a weakened one.
  NodeIteratorBase(Node*, unsigned what_to_show);
  unsigned AcceptNode(Node*, ExceptionState&);

 private:
  Member<Node> root_;
  unsigned what_to_show_;
  bool active_flag_ = false;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_DOM_NODE_ITERATOR_BASE_H_
