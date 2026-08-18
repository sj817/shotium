// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_DICTIONARY_BASE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_DICTIONARY_BASE_H_

#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace blink {

namespace bindings {

// InputDictionaryBase is the common base class for all dictionaries. The ones
// that also support conversion to Objects are inheriting DictionaryBase. Most
// importantly these classes provide a way to differentiate dictionary types in
// template specializations (i.e. are being used for constraints).
class PLATFORM_EXPORT InputDictionaryBase
    : public GarbageCollected<InputDictionaryBase> {
 public:
  InputDictionaryBase(const InputDictionaryBase&) = delete;
  InputDictionaryBase(const InputDictionaryBase&&) = delete;
  InputDictionaryBase& operator=(const InputDictionaryBase&) = delete;
  InputDictionaryBase& operator=(const InputDictionaryBase&&) = delete;

  virtual ~InputDictionaryBase() = default;
  virtual void Trace(Visitor*) const {}

 protected:
  InputDictionaryBase() = default;
};

// Upstream, DictionaryBase is the half of the split that can also convert
// itself *to* a script value: ToV8(), plus the three virtuals
// (TemplateKey/FillTemplateProperties/FillValues) that every generated
// dictionary implemented to build a v8::Object. All four are V8, and all four
// are gone.
//
// The class itself stays, rather than collapsing the hierarchy into
// InputDictionaryBase, because blink distinguishes the two in template
// constraints and the generated dictionaries name this one.
class PLATFORM_EXPORT DictionaryBase : public InputDictionaryBase {
 public:
  ~DictionaryBase() override = default;

 protected:
  DictionaryBase() = default;
};

}  // namespace bindings
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_DICTIONARY_BASE_H_
