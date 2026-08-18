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

#include "third_party/blink/renderer/platform/bindings/exception_state.h"

#include "base/check.h"
#include "base/check_op.h"
#include "base/notreached.h"
#include "third_party/blink/renderer/platform/bindings/exception_code.h"
#include "third_party/blink/renderer/platform/bindings/exception_context.h"
#include "third_party/blink/renderer/platform/bindings/exception_messages.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

NOINLINE void ExceptionState::ThrowSecurityError(
    const char* sanitized_message,
    const char* unsanitized_message) {
  ThrowSecurityError(String(sanitized_message), String(unsanitized_message));
}

NOINLINE void ExceptionState::ThrowRangeError(const char* message) {
  ThrowRangeError(String(message));
}

NOINLINE void ExceptionState::ThrowTypeError(const char* message) {
  ThrowTypeError(String(message));
}

NOINLINE void ExceptionState::ThrowSyntaxError(const char* message) {
  ThrowSyntaxError(String(message));
}

NOINLINE void ExceptionState::ThrowWasmCompileError(const char* message) {
  ThrowWasmCompileError(String(message));
}

NOINLINE void ExceptionState::ThrowDOMException(DOMExceptionCode exception_code,
                                                const char* message) {
  ThrowDOMException(exception_code, String(message));
}

void ExceptionState::SetExceptionInfo(ExceptionCode exception_code,
                                      const String& message) {
  had_exception_ = true;
  CHECK(exception_code);
  code_ = exception_code;
  message_ = message;
}

void ExceptionState::ThrowDOMException(DOMExceptionCode exception_code,
                                       const String& message) {
  // SecurityError is thrown via ThrowSecurityError, and _careful_ consideration
  // must be given to the data exposed to JavaScript via |sanitized_message|.
  DCHECK_NE(exception_code, DOMExceptionCode::kSecurityError);
#if DCHECK_IS_ON()
  DCHECK_AT(!assert_no_exceptions_, location_)
      << "DOMException should not be thrown.";
#endif

  SetExceptionInfo(ToExceptionCode(exception_code), message);
}

void ExceptionState::ThrowSecurityError(const String& sanitized_message,
                                        const String& unsanitized_message) {
#if DCHECK_IS_ON()
  DCHECK_AT(!assert_no_exceptions_, location_)
      << "SecurityError should not be thrown.";
#endif
  SetExceptionInfo(ToExceptionCode(DOMExceptionCode::kSecurityError),
                   sanitized_message);
}

void ExceptionState::ThrowRangeError(const String& message) {
#if DCHECK_IS_ON()
  DCHECK_AT(!assert_no_exceptions_, location_)
      << "RangeError should not be thrown.";
#endif
  SetExceptionInfo(ToExceptionCode(ESErrorType::kRangeError), message);
}

void ExceptionState::ThrowTypeError(const String& message) {
#if DCHECK_IS_ON()
  DCHECK_AT(!assert_no_exceptions_, location_)
      << "TypeError should not be thrown.";
#endif
  SetExceptionInfo(ToExceptionCode(ESErrorType::kTypeError), message);
}

void ExceptionState::ThrowSyntaxError(const String& message) {
#if DCHECK_IS_ON()
  DCHECK_AT(!assert_no_exceptions_, location_)
      << "SyntaxError should not be thrown.";
#endif
  SetExceptionInfo(ToExceptionCode(ESErrorType::kSyntaxError), message);
}

void ExceptionState::ThrowWasmCompileError(const String& message) {
#if DCHECK_IS_ON()
  DCHECK_AT(!assert_no_exceptions_, location_)
      << "WebAssembly.CompileError should not be thrown.";
#endif
  SetExceptionInfo(ToExceptionCode(ESErrorType::kWasmCompileError), message);
}

}  // namespace blink
