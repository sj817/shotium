// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SHOT_SHOT_API_H_
#define SHOT_SHOT_API_H_

// shot as a shared library, over an interface a C compiler can read.
//
// The engine is a Chromium fork: it is built with clang, its own libc++ and
// its own allocator, and none of that survives contact with a caller built by
// some other toolchain. A C++ interface would put std::string and std::vector
// in the seam, and their layout is a property of the standard library each
// side was compiled against. So the seam is C: opaque pointers, plain
// integers, NUL-terminated UTF-8, and no allocation crossing it in either
// direction that the owner does not free itself.
//
// This is the only header in shot/ with no chromium includes, and it must
// stay that way -- a caller compiles it having never heard of //base.
//
// The immediate caller is shotium's node addon, which needs a shot in the same
// process rather than in a child. It is not the only one this shape allows:
// ctypes, cgo and libloading all read this file as-is.
//
//   shot_engine* engine = NULL;
//   shot_buffer* error = NULL;
//   if (shot_engine_create("{}", &engine, &error) != SHOT_OK) {
//     fprintf(stderr, "%s\n", (const char*)shot_buffer_data(error));
//     shot_buffer_free(error);
//     return 1;
//   }
//   shot_buffer* png = NULL;
//   shot_engine_capture(engine, "{\"file\":\"https://example.com\"}",
//                       &png, &error);
//   ...
//   shot_buffer_free(png);
//   shot_engine_destroy(engine);

#include <stddef.h>
#include <stdint.h>

#if defined(_WIN32)
#if defined(SHOT_IMPLEMENTATION)
#define SHOT_EXPORT __declspec(dllexport)
#else
#define SHOT_EXPORT __declspec(dllimport)
#endif
#else
#if defined(SHOT_IMPLEMENTATION)
#define SHOT_EXPORT __attribute__((visibility("default")))
#else
#define SHOT_EXPORT
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Bumped when anything here changes meaning: a field the engine stops reading,
// a status that starts being returned where it was not, a lifetime rule that
// moves. Adding a JSON field neither side is required to send is not a change
// under this number, which is the reason the payloads are JSON.
#define SHOT_ABI_VERSION 1

// What the library was built as, which is not necessarily what the caller
// compiled against -- a prebuilt addon and a prebuilt engine are shipped as
// separate files and nothing stops them being separate versions. Check it
// before shot_engine_create() and say so plainly if it does not match.
SHOT_EXPORT int32_t shot_abi_version(void);

typedef enum {
  // The screenshot was taken, or the engine came up.
  SHOT_OK = 0,
  // The request or the options were not something the engine could act on:
  // malformed JSON, a field with the wrong type, a required field missing.
  // The error buffer names the field.
  SHOT_ERR_USAGE = 1,
  // The engine understood the request and could not answer it: the document
  // did not load, the selector matched nothing, the encoder failed.
  SHOT_ERR_CAPTURE = 2,
  // The call was made at a time it cannot be honoured -- a second engine while
  // one is live, a capture on an engine that failed to start. Blink is a
  // process-wide singleton, so this is not a transient condition to retry.
  SHOT_ERR_STATE = 3,
} shot_status;

// A live engine: a thread of its own, with blink on it.
//
// There can be one per process, ever -- not one at a time, one. Blink's
// initialisation writes process-wide statics that it has no path to undo, so
// destroying an engine frees what it can and does not give the process back
// the ability to make another. shot_engine_create() returns SHOT_ERR_STATE for
// the second attempt whether or not the first is still alive.
typedef struct shot_engine shot_engine;

// Bytes the library owns: an encoded image, or an error message.
//
// Always freed with shot_buffer_free() and never with the caller's free().
// The two sides of this interface do not share an allocator, and a buffer
// handed to the wrong one is a crash somewhere unrelated and much later.
//
// An error buffer is NUL-terminated, so it can be read as a C string. An image
// buffer is not; use shot_buffer_size().
typedef struct shot_buffer shot_buffer;

SHOT_EXPORT const uint8_t* shot_buffer_data(const shot_buffer* buffer);
SHOT_EXPORT size_t shot_buffer_size(const shot_buffer* buffer);
SHOT_EXPORT void shot_buffer_free(shot_buffer* buffer);

// Starts the engine and blocks until it is ready to render.
//
// `options_json` is one JSON object, and every field is optional:
//
//   {
//     "cacheDir":         "...",   // the HTTP cache; omitted means none
//     "userAgent":        "...",
//     "resourceDir":      "...",   // where shotium_data.pak and shotium_strings.pak
//                                  //   are; see below
//     "allowFileAccess":  false    // what a request that says nothing means
//   }
//
// resourceDir matters more than it looks. An executable finds its resource
// packs next to itself and a shared library cannot: on Linux the path base
// resolves for "the current module" is /proc/self/exe, which inside a library
// loaded by node names the node binary. A caller that ships the packs beside
// the library must say where they are. Omitting it keeps the executable's
// behaviour, which is right for a host that put everything in one directory.
//
// On failure `*out_engine` is left alone and `*out_error` receives a message
// the caller can print. `out_error` may be NULL if the caller does not want
// one; the status is still returned.
SHOT_EXPORT shot_status shot_engine_create(const char* options_json,
                                           shot_engine** out_engine,
                                           shot_buffer** out_error);

// Stops the engine and joins its thread. Any capture in flight is waited for
// -- there is no cancellation, because a half-rendered document is not
// something blink can be asked to abandon safely. See the note on
// shot_engine for what this does not undo.
SHOT_EXPORT void shot_engine_destroy(shot_engine* engine);

// One screenshot. Blocks until there is an answer.
//
// `request_json` is one ScreenshotOptions as it goes over shotium's wire --
// the same object shotium/src/lib/request.ts builds and shot/shot_request.cc
// parses, with no third spelling of it in between. `file` is required;
// everything else has a default.
//
//   {"file":"https://example.com","type":"png","width":1280,"height":720}
//
// Safe to call from any thread and from several at once, but not parallel in
// any useful sense: the calls are serialised onto the engine's thread, because
// blink renders one document at a time. A caller who wants two screenshots at
// once wants two processes, which is what shotium's pool is.
//
// Exactly one of `*out_image` and `*out_error` is set. A request that names a
// `path` for the engine to write itself sets an empty image rather than the
// bytes.
SHOT_EXPORT shot_status shot_engine_capture(shot_engine* engine,
                                            const char* request_json,
                                            shot_buffer** out_image,
                                            shot_buffer** out_error);

// Hands back what the engine is holding but can rebuild: blink's heap, the
// caches, PartitionAlloc's free lists. `release_working_set` additionally asks
// the OS to take the pages back, which costs a soft fault each on the next
// request and is worth it only when there may not be one soon.
//
// A resident worker does this for itself on a timer, because it can see its
// own request stream go quiet. A library cannot -- the queue belongs to the
// host -- so the host is the one that knows when a batch has ended, and this
// is how it says so.
SHOT_EXPORT void shot_engine_purge(shot_engine* engine,
                                   int32_t release_working_set);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SHOT_SHOT_API_H_
