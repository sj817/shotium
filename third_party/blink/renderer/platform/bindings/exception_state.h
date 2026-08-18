/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_EXCEPTION_STATE_H_
#define THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_EXCEPTION_STATE_H_

#include "base/check.h"
#include "base/compiler_specific.h"
#include "base/dcheck_is_on.h"
#include "base/location.h"
#include "third_party/blink/renderer/platform/bindings/exception_code.h"
#include "third_party/blink/renderer/platform/bindings/exception_context.h"
#include "third_party/blink/renderer/platform/platform_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

// ExceptionState is a scope-like class and provides a way to record a
// pending exception, with an option to cancel it. An exception message may be
// auto-generated.
//
// This used to report exceptions through V8: constructing a DOMException as a
// v8::Value (via a factory function injected from core/, since platform/
// cannot depend on core/'s DOMException) and throwing it with
// v8::Isolate::ThrowException. V8 is gone, so there is no JavaScript call
// stack left to unwind through and nothing left to catch the thrown value.
//
// What Blink core actually depends on ExceptionState for -- recording that
// "an exception happened here, with this code and this message" so the
// immediate C++ caller can check HadException() and bail out -- has nothing
// to do with V8, and keeps working exactly as before, through the exact same
// public methods (ThrowDOMException, ThrowTypeError, ThrowRangeError,
// ThrowSyntaxError, ThrowSecurityError, ThrowWasmCompileError, HadException,
// GetContext). Only the underlying mechanism -- record code + message
// locally instead of constructing and throwing a V8 exception object --
// changed.
class PLATFORM_EXPORT ExceptionState {
  STACK_ALLOCATED();

 public:
  ExceptionState() : ExceptionState(kEmptyContext) {}

  explicit ExceptionState(const ExceptionContext& context)
      : context_(context) {}

  ExceptionState(const ExceptionState&) = delete;
  ExceptionState& operator=(const ExceptionState&) = delete;

  ~ExceptionState() = default;

  // Throws a DOMException due to the given exception code.
  NOINLINE void ThrowDOMException(DOMExceptionCode, const String& message);

  // Throws a DOMException with SECURITY_ERR.
  NOINLINE void ThrowSecurityError(
      const String& sanitized_message,
      const String& unsanitized_message = String());

  // Throws an ECMAScript Error object.
  NOINLINE void ThrowRangeError(const String& message);
  NOINLINE void ThrowTypeError(const String& message);
  NOINLINE void ThrowSyntaxError(const String& message);

  // Throws WebAssembly Error object.
  NOINLINE void ThrowWasmCompileError(const String& message);

  // These overloads reduce the binary code size because the call sites do not
  // need the conversion by String::String(const char*) that is inlined at each
  // call site. As there are many call sites that pass in a const char*, this
  // size optimization is effective (32kb reduction as of June 2018).
  // See also https://crbug.com/849743
  NOINLINE void ThrowDOMException(DOMExceptionCode, const char* message);
  NOINLINE void ThrowSecurityError(const char* sanitized_message,
                                   const char* unsanitized_message = nullptr);
  NOINLINE void ThrowRangeError(const char* message);
  NOINLINE void ThrowTypeError(const char* message);
  NOINLINE void ThrowSyntaxError(const char* message);
  NOINLINE void ThrowWasmCompileError(const char* message);

  // Returns true if there is a pending exception.
  bool HadException() const { return had_exception_; }

  // Returns the context of what Web API is currently being executed.
  const ExceptionContext& GetContext() const { return context_; }

  ExceptionState& ReturnThis() { return *this; }

  // The code and message recorded by the most recent Throw*() call, if any.
  // These used to be readable only off DummyExceptionStateForTesting, which
  // pulled them back out of the V8 exception it had swallowed. Recording an
  // exception no longer goes through V8 at all, so every ExceptionState
  // records them directly, and DummyExceptionStateForTesting is now just a
  // named alias for "an ExceptionState nobody rethrows".
  ExceptionCode Code() const { return code_; }
  template <typename T>
  T CodeAs() const {
    return static_cast<T>(code_);
  }
  const String& Message() const { return message_; }

 protected:
  // Delegated constructor for NonThrowableExceptionState
  enum ForNonthrowable { kNonthrowable };
#if DCHECK_IS_ON()
  ExceptionState(const base::Location& location, ForNonthrowable)
      : context_(kEmptyContext),
        location_(location),
        assert_no_exceptions_(true) {}
#else
  explicit ExceptionState(ForNonthrowable) : context_(kEmptyContext) {}
#endif

  static constexpr ExceptionContext kEmptyContext{};

 private:
  void SetExceptionInfo(ExceptionCode, const String&);

  // The context represents what Web API is currently being executed.
  // In most cases, this is `kEmptyContext`. In the cases where
  // the generated bindings provide a non-empty context, the caller is
  // responsible for ensuring `context_` outlives this object.
  ExceptionContext context_;

  ExceptionCode code_ = 0;
  String message_;
  bool had_exception_ = false;

#if DCHECK_IS_ON()
  base::Location location_;
  bool assert_no_exceptions_ = false;
#endif
};

// NonThrowableExceptionState never allow call sites to throw an exception.
// Should be used if an exception must not be thrown.
class PLATFORM_EXPORT NonThrowableExceptionState final : public ExceptionState {
 public:
#if DCHECK_IS_ON()
  explicit NonThrowableExceptionState(
      base::Location location = base::Location::Current())
      : ExceptionState(location, kNonthrowable) {}
#else
  NonThrowableExceptionState() : ExceptionState(kNonthrowable) {}
#endif
};

// DummyExceptionStateForTesting ignores all thrown exceptions, same as a
// default-constructed ExceptionState (see IGNORE_EXCEPTION below); it exists
// as a named type so call sites can read back the code/message an ignored
// exception carried via Code()/CodeAs<T>()/Message() (inherited from
// ExceptionState above).
class PLATFORM_EXPORT DummyExceptionStateForTesting final
    : public ExceptionState {
 public:
  DummyExceptionStateForTesting() = default;
};

// Syntax sugar for a default-constructed ExceptionState, which ignores (but
// still records) any exception thrown into it.
// This can be used as a default value of an ExceptionState parameter like this:
//
//     Node* removeChild(Node*, ExceptionState& = IGNORE_EXCEPTION);
#define IGNORE_EXCEPTION (::blink::ExceptionState().ReturnThis())
#define IGNORE_EXCEPTION_FOR_TESTING IGNORE_EXCEPTION

// Syntax sugar for NonThrowableExceptionState.
// This can be used as a default value of an ExceptionState parameter like this:
//
//     Node* removeChild(Node*, ExceptionState& = ASSERT_NO_EXCEPTION);
#if DCHECK_IS_ON()
#define ASSERT_NO_EXCEPTION (::blink::NonThrowableExceptionState().ReturnThis())
#else
#define ASSERT_NO_EXCEPTION IGNORE_EXCEPTION
#endif
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_PLATFORM_BINDINGS_EXCEPTION_STATE_H_
