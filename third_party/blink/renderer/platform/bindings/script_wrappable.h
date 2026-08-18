/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_WRAPPABLE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_WRAPPABLE_H_

#include "third_party/blink/renderer/platform/bindings/name_client.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/type_traits.h"

namespace blink {

// ScriptWrappable used to be the C++/JavaScript bridge: every DOM/CSS/layout
// object derived from it so that ToV8()/Wrap()/AssociateWithWrapper() could
// create and cache a v8::Object "wrapper" for it (the main-world wrapper
// lived inline here; DOMDataStore held the wrappers for every other world),
// and so that a v8::Object could be unwrapped back into the C++ object via
// WrapperTypeInfo's RTTI.
//
// V8 has been removed. There is no wrapper to create, cache, look up, or
// trace, and WrapperTypeInfo -- which existed purely to let V8 recognize and
// downcast wrapped C++ objects -- is gone with it. What remains, and what
// every one of those DOM/CSS/layout classes still needs, is:
//   - a single root of blink's GarbageCollected<> hierarchy, so cppgc can see
//     the whole object graph through one common base, and
//   - a NameClient (cppgc::NameProvider) implementation, so the heap can
//     report *some* human-readable name for these objects when asked (heap
//     snapshots, GC debugging). The specific-to-each-interface name that
//     WrapperTypeInfo::interface_name used to supply came from generated
//     bindings code that no longer exists; subclasses that want a better name
//     than the generic one below are free to override GetHumanReadableName()
//     themselves, same as any other NameClient.
//
// ScriptWrappable is therefore now just `GarbageCollected<ScriptWrappable>`
// plus `NameClient`, and every class that used to derive from it keeps doing
// so unmodified: they pick up cppgc-managed lifetime through this one root,
// the same way they used to pick it up through it (indirectly, via V8's
// CppHeap wrapper base) before.
class PLATFORM_EXPORT ScriptWrappable
    : public GarbageCollected<ScriptWrappable>,
      public NameClient {
 public:
  ScriptWrappable(const ScriptWrappable&) = delete;
  ScriptWrappable& operator=(const ScriptWrappable&) = delete;
  ~ScriptWrappable() override = default;

  // NameClient. See the class comment: subclasses may override this to
  // report their own type name; this generic default is used otherwise.
  const char* GetHumanReadableName() const override;

  virtual void Trace(Visitor*) const;

 protected:
  ScriptWrappable() = default;
};

// DEFINE_WRAPPERTYPEINFO() used to generate a WrapperTypeInfo-backed
// GetWrapperTypeInfo()/GetStaticWrapperTypeInfo() pair (defined by the IDL
// code generator) for every interface. WrapperTypeInfo no longer exists, and
// nothing in the codebase should call GetWrapperTypeInfo() any more.
//
// The macro is kept -- as a no-op -- purely so the several hundred call
// sites across core/ that already say `DEFINE_WRAPPERTYPEINFO();` inside a
// class body keep compiling unmodified. It intentionally expands to a
// statement that is legal wherever the old macro was legal (a class member
// declaration terminated by the caller's own `;`), and generates nothing.
#define DEFINE_WRAPPERTYPEINFO() static_assert(true)

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_SCRIPT_WRAPPABLE_H_
