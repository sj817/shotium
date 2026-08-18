// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_platform.h"

#include <string_view>
#include <utility>

#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/mime_util.h"
#include "third_party/blink/public/mojom/mime/mime_registry.mojom-blink.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"
#include "ui/base/resource/resource_bundle.h"

namespace shot {
namespace {

// The browser half of MimeRegistry, in the same process.
//
// content::MimeRegistryImpl is one function, and this is that function:
// net::GetMimeTypeFromExtension over net's own table plus the platform's
// registry. There is no sandbox here, so the only thing the mojo pipe is
// crossing is a thread boundary.
class MimeRegistryImpl : public blink::mojom::blink::MimeRegistry {
 public:
  void GetMimeTypeFromExtension(
      const blink::String& extension,
      GetMimeTypeFromExtensionCallback callback) override {
    std::string mime_type;
    net::GetMimeTypeFromExtension(
        base::FilePath::FromUTF8Unsafe(extension.Utf8()).value(), &mime_type);
    std::move(callback).Run(blink::String::FromUtf8(mime_type));
  }
};

// Answers MimeRegistry and drops everything else, which is the truth about what
// is behind this broker.
//
// The receiver lives on a thread-pool sequence rather than on the blink thread,
// and it has to: GetMimeTypeFromExtension is a [Sync] method, so the caller
// blocks the blink thread until the reply arrives. A receiver on that same
// thread would be waiting for the thread that is waiting for it. The lookup
// also reads the Windows registry, which is why the sequence is MayBlock.
class ShotBrowserInterfaceBroker
    : public blink::ThreadSafeBrowserInterfaceBrokerProxy {
 public:
  ShotBrowserInterfaceBroker()
      : task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
            {base::MayBlock(), base::TaskPriority::USER_BLOCKING})) {}

 protected:
  ~ShotBrowserInterfaceBroker() override = default;

  void GetInterfaceImpl(mojo::GenericPendingReceiver receiver) override {
    if (auto mime_receiver =
            receiver.As<blink::mojom::blink::MimeRegistry>()) {
      task_runner_->PostTask(
          FROM_HERE,
          base::BindOnce(
              [](mojo::PendingReceiver<blink::mojom::blink::MimeRegistry> r) {
                mojo::MakeSelfOwnedReceiver(
                    std::make_unique<MimeRegistryImpl>(), std::move(r));
              },
              std::move(mime_receiver)));
      return;
    }
    // Dropping the receiver closes the pipe, which is how the caller learns
    // there is nothing on the other end -- the same answer the empty broker
    // gives, just without pretending otherwise.
  }

 private:
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
};

}  // namespace

ShotPlatform::ShotPlatform()
    : broker_(base::MakeRefCounted<ShotBrowserInterfaceBroker>()) {}
ShotPlatform::~ShotPlatform() = default;

bool ShotPlatform::HasDataResource(int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().HasDataResource(resource_id);
}

blink::WebData ShotPlatform::GetDataResource(
    int resource_id,
    ui::ResourceScaleFactor scale_factor) {
  // WebData wraps a span, and the bytes have to outlive it. ResourceBundle
  // owns the RefCountedMemory it hands back for the process's lifetime, so
  // copying the span into WebData is safe here.
  scoped_refptr<base::RefCountedMemory> bytes =
      ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytesForScale(
          resource_id, scale_factor);
  if (!bytes) {
    return blink::WebData();
  }
  return blink::WebData(*bytes);
}

std::string ShotPlatform::GetDataResourceString(int resource_id) {
  // LoadDataResourceString, not GetRawDataResource: blink's .grd entries are
  // compress="gzip"/"brotli" and this is the accessor that decompresses. Handing
  // back the compressed bytes would give the CSS parser a binary blob and a
  // document with no user-agent styles.
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceString(
      resource_id);
}

scoped_refptr<base::RefCountedMemory> ShotPlatform::GetDataResourceBytes(
    int resource_id) {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

blink::WebString ShotPlatform::DefaultLocale() {
  // The base class returns an empty WebString, and an empty one is not merely
  // uninformative: InitializePlatformLanguage() turns it into a null
  // AtomicString, and PlatformLanguage() then dereferences its null StringImpl
  // on the first document decoded.
  //
  // en-US is the honest answer for this binary rather than a placeholder: the
  // only strings it carries are blink_strings_en-US.pak and
  // ui_strings_en-US.pak (see //shot:shot_strings), so this is the locale it
  // actually has. The value is real input to rendering -- it picks the
  // language-sensitive font fallback and the default quote characters -- so it
  // has to agree with what is packed.
  return blink::WebString::FromUtf8("en-US");
}

blink::ThreadSafeBrowserInterfaceBrokerProxy*
ShotPlatform::GetBrowserInterfaceBroker() {
  return broker_.get();
}

}  // namespace shot
