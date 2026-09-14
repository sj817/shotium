// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_PLATFORM_H_
#define SHOT_SHOT_PLATFORM_H_

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/platform/platform.h"

namespace shot {

// shot's blink::Platform.
//
// blink::Platform has no pure virtual methods -- every hook has a default that
// answers "not available" -- so an embedder only overrides what it genuinely
// provides. shot provides the packed data resources, because blink's
// user-agent stylesheet is one of them: without it every document would lay
// out with no default styles at all, which is not a smaller renderer, it is a
// wrong one. And the locale, for the same reason (see DefaultLocale()).
//
// It used to provide a browser interface broker as well, answering exactly
// one interface -- mojom::MimeRegistry -- because MIMETypeRegistry proxied
// extension lookups to the browser over a synchronous mojo call. That lookup
// now runs in-process (mime_type_registry.cc), so the broker stays at
// blink's default, which drops every interface request on the floor: the
// truthful answer for a process with no browser behind it.
//
// Everything else stays at the default. That is not a stub: the defaults are
// blink's own statement of what an embedder without a browser process can do,
// and this binary is mostly exactly that.
class ShotPlatform : public blink::Platform {
 public:
  ShotPlatform();
  ShotPlatform(const ShotPlatform&) = delete;
  ShotPlatform& operator=(const ShotPlatform&) = delete;
  ~ShotPlatform() override;

  // blink::Platform:
  bool HasDataResource(int resource_id) const override;
  blink::WebData GetDataResource(
      int resource_id,
      ui::ResourceScaleFactor scale_factor) override;
  std::string GetDataResourceString(int resource_id) override;
  scoped_refptr<base::RefCountedMemory> GetDataResourceBytes(
      int resource_id) override;
  blink::WebString DefaultLocale() override;
};

}  // namespace shot

#endif  // SHOT_SHOT_PLATFORM_H_
