// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_DOM_WINDOW_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_DOM_WINDOW_H_
#include "base/memory/scoped_refptr.h"
#include "services/network/public/mojom/cross_origin_opener_policy.mojom-blink.h"
#include "third_party/blink/public/common/messaging/message_port_channel.h"
#include "third_party/blink/public/common/tokens/tokens.h"
#include "third_party/blink/public/mojom/frame/frame.mojom-blink-forward.h"
#include "third_party/blink/public/mojom/messaging/delegated_capability.mojom-blink.h"
#include "third_party/blink/public/mojom/use_counter/metrics/web_feature.mojom-blink-forward.h"
#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/dom/events/event_target.h"
#include "third_party/blink/renderer/core/frame/frame.h"
#include "third_party/blink/renderer/core/url/dom_origin_utils.h"
#include "third_party/blink/renderer/platform/heap/collection_support/heap_vector.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/mojo/heap_mojo_remote.h"
#include "third_party/blink/renderer/platform/wtf/assertions.h"

namespace blink {

class ContextLifecycleNotifier;
class InputDeviceCapabilitiesConstants;
class LocalDOMWindow;
class Location;
class SecurityOrigin;
class UserActivation;

// DOMWindow is an abstract class of Window interface implementations.
// We have two derived implementation classes;  LocalDOMWindow and
// RemoteDOMWindow.
//
// TODO(tkent): Rename DOMWindow to Window. The class was named as 'DOMWindow'
// because WebKit already had KJS::Window.  We have no reasons to avoid
// blink::Window now.
class CORE_EXPORT DOMWindow : public EventTarget, public DOMOriginUtils {
  DEFINE_WRAPPERTYPEINFO();

 public:
  enum class CrossDocumentAccessPolicy { kAllowed, kDisallowed };

  ~DOMWindow() override;

  // DOMOriginUtils overrides:
  DOMOrigin* GetDOMOrigin(LocalDOMWindow* accessing_window) const override;

  Frame* GetFrame() const {
    // A Frame is typically reused for navigations. If |frame_| is not null,
    // two conditions must always be true:
    // - |frame_->domWindow()| must point back to this DOMWindow. If it does
    //   not, it is easy to introduce a bug where script execution uses the
    //   wrong DOMWindow (which may be cross-origin).
    // - |frame_| must be attached, i.e. |frame_->page()| must not be null.
    //   If |frame_->page()| is null, this indicates a bug where the frame was
    //   detached but |frame_| was not set to null. This bug can lead to
    //   issues where executing script incorrectly schedules work on a detached
    //   frame.
    SECURITY_DCHECK(!frame_ ||
                    (frame_->DomWindow() == this && frame_->GetPage()));
    return frame_.Get();
  }

  // GarbageCollected overrides:
  void Trace(Visitor*) const override;

  virtual bool IsLocalDOMWindow() const = 0;

  // EventTarget overrides:
  const AtomicString& InterfaceName() const override;
  const DOMWindow* ToDOMWindow() const override;
  bool IsWindowOrWorkerGlobalScope() const final;

  // Cross-origin DOM Level 0
  Location* location() const;
  bool closed() const;
  unsigned length() const;

  // Self-referential attributes
  DOMWindow* self() const;
  DOMWindow* window() const;
  DOMWindow* frames() const;

  DOMWindow* opener() const;
  DOMWindow* parent() const;
  DOMWindow* top() const;

  void Close(LocalDOMWindow* incumbent_window);

  String SanitizedCrossDomainAccessErrorMessage(
      const LocalDOMWindow* accessing_window,
      CrossDocumentAccessPolicy cross_document_access) const;
  String CrossDomainAccessErrorMessage(
      const LocalDOMWindow* accessing_window,
      CrossDocumentAccessPolicy cross_document_access) const;

  // FIXME: When this DOMWindow is no longer the active DOMWindow (i.e.,
  // when its document is no longer the document that is displayed in its
  // frame), we would like to zero out |frame_| to avoid being confused
  // by the document that is currently active in |frame_|.
  // See https://bugs.webkit.org/show_bug.cgi?id=62054
  bool IsCurrentlyDisplayedInFrame() const;

  InputDeviceCapabilitiesConstants* GetInputDeviceCapabilities();

 protected:
  explicit DOMWindow(Frame&);

  void DisconnectFromFrame() { frame_ = nullptr; }

 private:
  Member<Frame> frame_;
  Member<InputDeviceCapabilitiesConstants> input_capabilities_;
  mutable Member<Location> location_;

  // Set to true when Close() has been called. Needed for
  // |window.closed| determinism; having it return 'true'
  // only after the layout widget's deferred window close
  // operation has been performed.
  bool window_is_closing_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_DOM_WINDOW_H_
