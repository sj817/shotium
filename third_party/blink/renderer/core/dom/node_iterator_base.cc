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

#include "third_party/blink/renderer/core/dom/node_iterator_base.h"

#include "base/auto_reset.h"
#include "third_party/blink/renderer/core/dom/node.h"
#include "third_party/blink/renderer/core/execution_context/execution_context.h"
#include "third_party/blink/renderer/core/frame/web_feature.h"
#include "third_party/blink/renderer/platform/bindings/exception_state.h"
#include "third_party/blink/renderer/platform/instrumentation/use_counter.h"

namespace blink {

NodeIteratorBase::NodeIteratorBase(Node* root_node, unsigned what_to_show)
    : root_(root_node), what_to_show_(what_to_show) {}

unsigned NodeIteratorBase::AcceptNode(Node* node,
                                      ExceptionState& exception_state) {
  // DOM 6. Traversal
  // https://dom.spec.whatwg.org/#traversal
  // Each NodeIterator and TreeWalker object has an associated active flag to
  // avoid recursive invocations.
  if (active_flag_) {
    // 1. If the active flag is set, then throw an "InvalidStateError"
    // DOMException.
    exception_state.ThrowDOMException(DOMExceptionCode::kInvalidStateError,
                                      "Filter function can't be recursive");
    return kFilterReject;
  }

  // 2. Let n be node’s nodeType attribute value minus 1.
  // 3. If the nth bit (where 0 is the least significant bit) of whatToShow is
  // not set, then return FILTER_SKIP.
  //
  // The bit twiddling here is done to map DOM node types, which are given as
  // integers from 1 through 14, to whatToShow bit masks.
  if (!(((1 << (node->getNodeType() - 1)) & what_to_show_)))
    return kFilterSkip;

  // 4. If filter is null, then return FILTER_ACCEPT.
  //
  // The filter is always null. Steps 5 to 8 called the script filter's
  // acceptNode() under the active flag and returned what it said.
  return kFilterAccept;
}

void NodeIteratorBase::Trace(Visitor* visitor) const {
  visitor->Trace(root_);
}

}  // namespace blink
