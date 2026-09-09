// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Resolve Node-API against the host executable, including a renamed node.exe.
#include <windows.h>

#include <delayimp.h>

#include <cstring>

#include "base/compiler_specific.h"

namespace {
FARPROC WINAPI LoadNode(unsigned notification, PDelayLoadInfo info) {
  // SAFETY: the Windows delay-load helper supplies a NUL-terminated DLL name.
  if (notification == dliNotePreLoadLibrary &&
      UNSAFE_BUFFERS(std::strcmp(info->szDll, "node.exe")) == 0) {
    return reinterpret_cast<FARPROC>(GetModuleHandleW(nullptr));
  }
  return nullptr;
}
}  // namespace
extern "C" const PfnDliHook __pfnDliNotifyHook2 = LoadNode;
