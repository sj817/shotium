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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CORE_INITIALIZER_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CORE_INITIALIZER_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/platform/wtf/allocator/allocator.h"

namespace mojo {
class BinderMap;
}

namespace blink {

class Document;
class LocalFrame;
class PictureInPictureController;

class CORE_EXPORT CoreInitializer {
  USING_FAST_MALLOC(CoreInitializer);

 public:
  // Initialize must be called before GetInstance.
  static CoreInitializer& GetInstance() {
    DCHECK(instance_);
    return *instance_;
  }

  CoreInitializer(const CoreInitializer&) = delete;
  CoreInitializer& operator=(const CoreInitializer&) = delete;
  virtual ~CoreInitializer() = default;

  // Should be called by clients before trying to create Frames.
  virtual void Initialize();

  // Called on startup to register Mojo interfaces that for control messages,
  // e.g. messages that are not routed to a specific frame.
  virtual void RegisterInterfaces(mojo::BinderMap&) = 0;
  // Methods defined in CoreInitializer and implemented by ModulesInitializer to
  // bypass the inverted dependency from core/ to modules/.
  // Mojo Interfaces registered with LocalFrame
  virtual void InitLocalFrame(LocalFrame&) const = 0;
  // Supplements installed on a frame using ChromeClient
  virtual void InstallSupplements(LocalFrame&) const = 0;
  // CreateMediaControls(), CreateRemotePlaybackClient() and
  // CreateWebMediaPlayer() were here. They were how core asked modules/ for
  // the pieces of media playback that lived there. The media player is cut
  // out of HTMLMediaElement, so nothing calls them, and their return types
  // -- MediaControls, RemotePlaybackClient, WebMediaPlayer -- name classes
  // that no longer exist.
  virtual PictureInPictureController* CreatePictureInPictureController(
      Document&) const = 0;

  // GetFileSystemManager() was here. Its only caller was
  // File::CreateForFileSystemFile(), on the filesystem: URL snapshot path of
  // the JS File System API -- not the loading path. blink.mojom.FileSystemManager
  // is deleted along with the browser side that served it.

 protected:
  // CoreInitializer is only instantiated by subclass ModulesInitializer.
  CoreInitializer() = default;

 private:
  static CoreInitializer* instance_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CORE_INITIALIZER_H_
