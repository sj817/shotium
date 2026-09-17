// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_PLATFORM_H_
#define SHOT_SHOT_PLATFORM_H_

#include <memory>
#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "build/build_config.h"
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
// Linux is the one exception to the "no browser process" defaults: Blink's
// local() font lookup is expressed through WebSandboxSupport because Chrome's
// renderer normally asks its browser-side font service. Shot is not sandboxed,
// so its implementation performs the same Fontconfig lookup in-process and
// gates it with the capture's existing allowFileAccess permission.
class ShotPlatform : public blink::Platform {
 public:
  ShotPlatform();
  ShotPlatform(const ShotPlatform&) = delete;
  ShotPlatform& operator=(const ShotPlatform&) = delete;
  ~ShotPlatform() override;

  // blink::Platform:
#if BUILDFLAG(IS_LINUX)
  blink::WebSandboxSupport* GetSandboxSupport() override;
#endif
  bool HasDataResource(int resource_id) const override;
  blink::WebData GetDataResource(
      int resource_id,
      ui::ResourceScaleFactor scale_factor) override;
  std::string GetDataResourceString(int resource_id) override;
  scoped_refptr<base::RefCountedMemory> GetDataResourceBytes(
      int resource_id) override;
  blink::WebString DefaultLocale() override;

#if BUILDFLAG(IS_LINUX)
 private:
  std::unique_ptr<blink::WebSandboxSupport> sandbox_support_;
#endif
};

}  // namespace shot

#endif  // SHOT_SHOT_PLATFORM_H_
