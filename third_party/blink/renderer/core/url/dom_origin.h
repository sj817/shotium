// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_URL_DOM_ORIGIN_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_URL_DOM_ORIGIN_H_

#include "base/types/pass_key.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/bindings/script_wrappable.h"
#include "third_party/blink/renderer/platform/weborigin/security_origin.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class CORE_EXPORT DOMOrigin final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  // Creates a unique opaque origin:
  static DOMOrigin* Create();

  static DOMOrigin* Create(scoped_refptr<const SecurityOrigin>);

  DOMOrigin(base::PassKey<DOMOrigin>, scoped_refptr<const SecurityOrigin>);
  ~DOMOrigin() override;

  bool opaque() const;

  bool isSameOrigin(const DOMOrigin* other) const;
  bool isSameSite(const DOMOrigin* other) const;

  void Trace(Visitor*) const override;

  // Expose the internal `SecurityOrigin` for unit tests:
  const SecurityOrigin* GetOriginForTesting() const { return origin_.get(); }

 private:
  const scoped_refptr<const SecurityOrigin> origin_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_URL_DOM_ORIGIN_H_
