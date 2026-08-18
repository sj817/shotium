// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_URL_URL_SEARCH_PARAMS_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_URL_URL_SEARCH_PARAMS_H_

#include <cstdint>
#include <utility>

#include "base/dcheck_is_on.h"
#include "base/gtest_prod_util.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/network/encoded_form_data.h"
#include "third_party/blink/renderer/platform/wtf/forward.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class ExecutionContext;
class URL;

class CORE_EXPORT URLSearchParams final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  static URLSearchParams* Create(const String& query_string,
                                 URL* url_object = nullptr) {
    return MakeGarbageCollected<URLSearchParams>(query_string, url_object);
  }

  explicit URLSearchParams(const String&, URL* = nullptr);
  ~URLSearchParams() override;

  // URLSearchParams interface methods
  String toString() const;
  uint32_t size() const;
  void append(const String& name, const String& value);
  void deleteAllWithNameOrTuple(ExecutionContext* execution_context,
                                const String& name);
  void deleteAllWithNameOrTuple(ExecutionContext* execution_context,
                                const String& name,
                                const String& val);
  String get(const String&) const;
  Vector<String> getAll(const String&) const;
  bool has(ExecutionContext* execution_context, const String& name) const;
  bool has(ExecutionContext* execution_context,
           const String& name,
           const String& val) const;
  void set(const String& name, const String& value);
  void sort();
  void SetInputWithoutUpdate(const String&);

  // Internal helpers
  scoped_refptr<EncodedFormData> ToEncodedFormData() const;
  const Vector<std::pair<String, String>>& Params() const { return params_; }

#if DCHECK_IS_ON()
  URL* UrlObject() const;
#endif

  void Trace(Visitor*) const override;

 private:
  FRIEND_TEST_ALL_PREFIXES(URLSearchParamsTest, EncodedFormData);

  void RunUpdateSteps();
  void EncodeAsFormData(Vector<char>&) const;

  void AppendWithoutUpdate(const String& name, const String& value);

  Vector<std::pair<String, String>> params_;

  WeakMember<URL> url_object_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_URL_URL_SEARCH_PARAMS_H_
