// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef THIRD_PARTY_BLINK_RENDERER_MODULES_MODULES_INITIALIZER_H_
#define THIRD_PARTY_BLINK_RENDERER_MODULES_MODULES_INITIALIZER_H_

#include "third_party/blink/renderer/core/core_initializer.h"
#include "third_party/blink/renderer/modules/modules_export.h"

namespace blink {

// CoreInitializer exists so that core/ can reach features that live in
// modules/ without depending on modules/ directly: every method below is a
// hole that core/ calls through.
//
// All 120 module directories have been deleted, so every hole is now plugged
// with a no-op. This class is kept rather than folded into core/ because it is
// the type that controller/BlinkInitializer derives from, and because it
// documents, in one place, exactly which capabilities a Shot renderer does not
// have: media playback and its controls, picture-in-picture, remote playback,
// WebGL and WebGPU canvas contexts, session and local storage, the File System
// API, manifests, screen orientation, and the modules-side inspector agents.
//
// The methods are not pure-virtual holes any more, but they are still
// overrides, so removing one from CoreInitializer will fail the build here
// rather than silently leaving a stale definition behind.
class MODULES_EXPORT ModulesInitializer : public CoreInitializer {
 public:
  void Initialize() override;

 protected:
  void InitLocalFrame(LocalFrame&) const override;
  void OnClearWindowObjectInMainWorld(Document&,
                                      const Settings&) const override;

 private:
  void InstallSupplements(LocalFrame&) const override;
  PictureInPictureController* CreatePictureInPictureController(
      Document&) const override;
  void InitInspectorAgentSession(DevToolsSession*,
                                 InspectorDOMAgent*,
                                 InspectedFrames*,
                                 Page*) const override;
  void InitWorkerInspectorAgentSession(DevToolsSession*,
                                       WorkerGlobalScope*) const override;
  void InitServiceWorkerGlobalScope(ServiceWorkerGlobalScope&) const override;
  void ProvideModulesToPage(Page&,
                            const SessionStorageNamespaceId&) const override;
  void ForceNextWebGLContextCreationToFail() const override;

  void CollectAllGarbageForAnimationAndPaintWorkletForTesting() const override;

  void CloneSessionStorage(
      Page* clone_from_page,
      const SessionStorageNamespaceId& clone_to_namespace) override;
  void EvictSessionStorageCachedData(Page*) override;

  void DidChangeManifest(LocalFrame&) override;
  void NotifyOrientationChanged(LocalFrame&) override;
  void DidUpdateScreens(LocalFrame&, const display::ScreenInfos&) override;
  void SetLocalStorageArea(LocalFrame& frame,
                           mojo::PendingRemote<mojom::blink::StorageArea>
                               local_storage_area) override;
  void SetSessionStorageArea(LocalFrame& frame,
                             mojo::PendingRemote<mojom::blink::StorageArea>
                                 session_storage_area) override;
  // GetFileSystemManager() override was here. CoreInitializer no longer
  // declares it -- see the comment in core/core_initializer.h -- so this
  // override has nothing left to override.
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_MODULES_MODULES_INITIALIZER_H_
