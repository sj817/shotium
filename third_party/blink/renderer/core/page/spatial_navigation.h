/*
 * Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2009 Antonio Gomes <tonikitoo@webkit.org>
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
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SPATIAL_NAVIGATION_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SPATIAL_NAVIGATION_H_

#include "third_party/blink/renderer/core/core_export.h"

namespace blink {

class LocalFrame;
class Node;

// SpatialNavigationController and the focus-candidate geometry that backed it
// are not built in shotium; only the two helpers below still have callers
// (Element::CanBeKeyboardFocusableScroller and the form-control keyboard
// handlers).
enum class SpatialNavigationDirection { kNone, kUp, kRight, kDown, kLeft };

CORE_EXPORT bool IsSpatialNavigationEnabled(const LocalFrame*);

// Note this function might trigger UpdateStyleAndLayout.
CORE_EXPORT bool IsScrollableNode(const Node*, SpatialNavigationDirection);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_PAGE_SPATIAL_NAVIGATION_H_
