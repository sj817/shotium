// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/modules/modules_initializer.h"

namespace blink {

void ModulesInitializer::Initialize() {
  // Upstream this reserved static string capacity for the modules event
  // interface, event target and IndexedDB name tables, initialised those
  // tables, installed EventModulesFactory as Document's event factory,
  // registered the modules V8 bindings, the accessibility cache, the CSS
  // paint/background-color/clip-path image generators, the MediaSource
  // registry, and the 2D/WebGL/WebGPU/ImageBitmap rendering context factories
  // on HTMLCanvasElement and OffscreenCanvas.
  //
  // Every one of those lives in a modules/ directory that no longer exists.
  // Core's own event factory, installed by CoreInitializer, covers the events
  // a static document dispatches.
  CoreInitializer::Initialize();
}

// Everything below is a hole core/ calls through to reach modules/. With
// modules/ deleted, each one answers "this renderer does not do that".
//
// The pointer-returning methods return null because their callers already
// handle a null result: core/ has to cope with an embedder that declines to
// provide media controls or a picture-in-picture controller. The void methods
// return without doing anything for the same reason.

void ModulesInitializer::InstallSupplements(LocalFrame&) const {}

PictureInPictureController*
ModulesInitializer::CreatePictureInPictureController(Document&) const {
  return nullptr;
}

}  // namespace blink
