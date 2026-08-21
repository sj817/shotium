// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_SERVER_H_
#define SHOT_SHOT_SERVER_H_

namespace shot {

// The resident worker: read a request off stdin, render it, write the result to
// stdout, repeat until stdin closes.
//
// Framing, in both directions, is a 4-byte little-endian length followed by
// that many bytes. A request is one frame of UTF-8 JSON matching shotium's
// ScreenshotOptions. A response is two frames: a JSON header, then the encoded
// image, which is empty when the header reports a failure or when the worker
// was asked to write the file itself.
//
//   ->  [len][{"file":"...","width":1248,...}]
//   <-  [len][{"ok":true,"bytes":97756}]  [len][<PNG bytes>]
//   <-  [len][{"ok":false,"error":"..."}] [0]
//
// Length-prefixed rather than line-delimited because the payload is binary; a
// newline in a PNG is not a message boundary. The handles are opened as
// platform files rather than through the C runtime, so there is no text-mode
// translation on Windows to corrupt those bytes.
//
// Diagnostics go to stderr, which is why the protocol can own stdout: logging
// is already configured for stderr in main(), and a stray printf into the
// stream would desynchronise the framing permanently.
//
// There is no shutdown message. The supervisor ends a worker by closing the
// pipe or killing the process, and a worker that dies mid-request is
// indistinguishable to it from one that never answered -- which is the point,
// since a crash has to look like a timeout for the retry path to be simple.
//
// `default_allow_file_access` is what an incoming request means when it says
// nothing about allowFileAccess. It comes from the command line that started
// the worker rather than from the request, because the process that renders
// a page for a stranger should not be taking that instruction from the
// stranger. See shot_options.cc for the flag.
//
// Requires a live ShotRuntime on this thread. Returns a process exit code.
int RunServer(bool default_allow_file_access);

}  // namespace shot

#endif  // SHOT_SHOT_SERVER_H_
