// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_UNION_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_UNION_BASE_H_

#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"

namespace blink {

// Upstream this lives in the deleted bindings/core/v8/native_value_traits_impl.h
// and is the placeholder stored by an IDL union that has an `undefined` member:
// there is nothing to remember, only which arm of the union is active. The
// struct never had any V8 in it -- only the ToV8Traits<IDLUndefined>
// specialisation that consumed it did -- so the generated union classes can go
// on naming it, and so can the two call sites in core that construct one
// (style_property_map_read_only_main_thread.cc and
// paint_worklet_style_property_map.cc).
//
// It is parked here, rather than in a header of its own, because every union
// class already includes this one and nothing else needs it. If
// native_value_traits_impl.h is ever restored, delete this copy: two
// definitions of the same class in different headers is an ODR violation, not
// a merge conflict, so it will not announce itself.
struct ToV8UndefinedGenerator {
  DISALLOW_NEW();
  using ImplType = ToV8UndefinedGenerator;
};

namespace bindings {

// UnionBase is the common base class of all the IDL union classes.  Most
// importantly this class provides a way of type dispatching (e.g. overload
// resolutions, SFINAE technique, etc.) so that it's possible to distinguish
// IDL unions from anything else.  Also it provides a common implementation of
// IDL unions.
class PLATFORM_EXPORT UnionBase : public GarbageCollected<UnionBase> {
 public:
  virtual ~UnionBase() = default;

  virtual void Trace(Visitor*) const {}

 protected:
  // Upstream also declares
  //
  //   static void ThrowTypeErrorNotOfType(ExceptionState&, const char*);
  //
  // whose only caller was the generated Create(), the V8-value-to-union
  // conversion. Create() is gone, so the last caller is gone, and with it the
  // only reason for union_base.cc to exist. The declaration goes too rather
  // than being left to dangle: a protected static with no definition is an
  // undefined symbol waiting for whoever calls it next.

  UnionBase() = default;
};

}  // namespace bindings

// Upstream also defines bindings::OptimizedReturnProxy<T> here: a stack
// allocated holder that a generated getter could return instead of allocating
// a union on the heap, by doing the ToV8 conversion eagerly at the creation
// site. Its entire body is v8::MaybeLocal<v8::Value>, filled in by
// T::DirectToV8(ScriptState*, ...), and both of those went with V8, so there
// is nothing left to keep. Nothing outside the deleted generated bindings ever
// named it (`git grep OptimizedReturnProxy` over the tree finds no other
// reference), so dropping it costs no caller.

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_UNION_BASE_H_
