// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_PLATFORM_H_
#define SHOT_SHOT_PLATFORM_H_

#include <string>

#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/public/common/thread_safe_browser_interface_broker_proxy.h"
#include "third_party/blink/public/platform/platform.h"

namespace shot {

// shot's blink::Platform.
//
// blink::Platform has no pure virtual methods -- every hook has a default that
// answers "not available" -- so an embedder only overrides what it genuinely
// provides. shot provides two things.
//
// The packed data resources, because blink's user-agent stylesheet is one of
// them. Without it every document would lay out with no default styles at all,
// which is not a smaller renderer, it is a wrong one.
//
// And a browser interface broker that answers exactly one interface,
// mojom::MimeRegistry. That one is not optional either: MIMETypeRegistry
// proxies extension lookups to the browser because a sandboxed renderer cannot
// read the registry, and with nothing on the far end every lookup returns
// empty. CSSStyleSheetResource::CanUseSheet requires a file: stylesheet to have
// an extension that maps to text/css, so an unanswered lookup means every
// external stylesheet loaded over file: is fetched, parsed and then discarded --
// silently, with the document laid out as though the author had written no CSS.
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
  blink::ThreadSafeBrowserInterfaceBrokerProxy* GetBrowserInterfaceBroker()
      override;

 private:
  scoped_refptr<blink::ThreadSafeBrowserInterfaceBrokerProxy> broker_;
};

}  // namespace shot

#endif  // SHOT_SHOT_PLATFORM_H_
