// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_EXCEPTION_CONTEXT_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_EXCEPTION_CONTEXT_H_

#include "base/check_op.h"
#include "base/dcheck_is_on.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// The kind of Web API entry point that is currently being executed, i.e.
// what shape of call led to a (potential) exception.
//
// This used to be `v8::ExceptionContext`, an enum owned by V8 (its values
// drove how V8 formatted an exception's message). With V8 removed there is
// no V8 formatting to drive, but ExceptionState and ExceptionMessages still
// use the kind of entry point to build a human-readable "Failed to
// execute/construct/get/set ..." message, so the enum itself -- with the same
// members -- is reproduced here under Blink's own name.
enum class ExceptionContextType {
  kUnknown,
  kConstructor,
  kOperation,
  kIndexedGetter,
  kIndexedSetter,
  kIndexedDeleter,
  kIndexedDefiner,
  kIndexedDescriptor,
  kIndexedQuery,
  kNamedGetter,
  kNamedSetter,
  kNamedDeleter,
  kNamedDefiner,
  kNamedDescriptor,
  kNamedQuery,
  kNamedEnumerator,
  kAttributeGet,
  kAttributeSet,
};

// ExceptionContext stores context information about what Web API throws an
// exception.
//
// Note that ExceptionContext accepts only string literals as its string
// parameters.
class PLATFORM_EXPORT ExceptionContext final {
  DISALLOW_NEW();

 public:
  // Note `class_name` and `property_name` accept only string literals.
  ExceptionContext(ExceptionContextType type,
                   const char* class_name,
                   const char* property_name)
      : type_(type), class_name_(class_name), property_name_(property_name) {
#if DCHECK_IS_ON()
    switch (type) {
      case ExceptionContextType::kAttributeGet:
      case ExceptionContextType::kAttributeSet:
      case ExceptionContextType::kOperation:
      case ExceptionContextType::kIndexedGetter:
      case ExceptionContextType::kIndexedDescriptor:
      case ExceptionContextType::kIndexedSetter:
      case ExceptionContextType::kIndexedDefiner:
      case ExceptionContextType::kIndexedDeleter:
      case ExceptionContextType::kIndexedQuery:
      case ExceptionContextType::kNamedGetter:
      case ExceptionContextType::kNamedDescriptor:
      case ExceptionContextType::kNamedSetter:
      case ExceptionContextType::kNamedDefiner:
      case ExceptionContextType::kNamedDeleter:
      case ExceptionContextType::kNamedQuery:
        DCHECK(class_name);
        DCHECK(property_name);
        break;
      case ExceptionContextType::kConstructor:
      case ExceptionContextType::kNamedEnumerator:
        DCHECK(class_name);
        break;
      case ExceptionContextType::kUnknown:
        break;
    }
#endif  // DCHECK_IS_ON()
  }

  constexpr ExceptionContext()
      : type_(ExceptionContextType::kUnknown),
        class_name_(nullptr),
        property_name_(nullptr) {}

  ExceptionContext(const ExceptionContext&) = default;
  ExceptionContext(ExceptionContext&&) = default;
  ExceptionContext& operator=(const ExceptionContext&) = default;
  ExceptionContext& operator=(ExceptionContext&&) = default;

  ~ExceptionContext() = default;

  ExceptionContextType GetType() const { return type_; }
  const char* GetClassName() const { return class_name_; }
  const char* GetPropertyName() const { return property_name_; }

 private:
  ExceptionContextType type_;
  const char* class_name_ = nullptr;
  const char* property_name_ = nullptr;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_EXCEPTION_CONTEXT_H_
