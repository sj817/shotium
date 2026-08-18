// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_V8_BINDING_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_V8_BINDING_H_

// What is left of v8_binding.h.
//
// Upstream this is the V8 conversion layer -- ToV8Traits, NativeValueTraits,
// exception plumbing, the whole of it -- and all of that is gone.
//
// These three enums are not part of it. They are the *return values* of the
// interceptor callbacks that blink implements on its own classes:
//
//     IndexedPropertySetterResult CSSUnparsedValue::AnonymousIndexedSetter(...)
//     NamedPropertySetterResult HTMLSelectElement::AnonymousNamedSetter(...)
//
// The callbacks are declared in core, and only a v8 property interceptor ever
// called them, so nothing calls them now -- but they are declared in headers
// that plenty of live code includes, and the declarations need a return type.
// Three enum definitions are a much smaller thing to carry than editing every
// interface that has an indexed or named property.

#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
namespace blink {

enum class IndexedPropertySetterResult {
  kDidNotIntercept,  // Fallback to the default set operation.
  kIntercepted,      // Intercepted regardless of whether it succeeded or not.
};

enum class NamedPropertySetterResult {
  kDidNotIntercept,  // Fallback to the default set operation.
  kIntercepted,      // Intercepted regardless of whether it succeeded or not.
};

enum class NamedPropertyDeleterResult {
  kDidNotIntercept,  // Fallback to the default delete operation.
  kDidNotDelete,
  kDeleted,
};


// Replaces unmatched UTF-16 surrogates with U+FFFD, per
// https://webidl.spec.whatwg.org/#dfn-obtain-unicode. Not a V8 function; see
// v8_binding.cc.
PLATFORM_EXPORT String ReplaceUnmatchedSurrogates(String);

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_V8_BINDING_H_
