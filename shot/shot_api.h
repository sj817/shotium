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
#define SHOT_ABI_VERSION 3

// What the library was built as, which is not necessarily what the caller
// compiled against -- a prebuilt addon and a prebuilt engine are shipped as
// separate files and nothing stops them being separate versions. Check it
// before shot_engine_create() and say so plainly if it does not match.
SHOT_EXPORT int32_t shot_abi_version(void);

// Out parameters: every function below clears all of its outputs before it
// does anything else, so a caller may reuse the same variables across calls
// without zeroing them in between and will never be handed the previous
// call's buffer back. Where a function says exactly one of two outputs is
// set, the other one is NULL rather than untouched, which is what makes that
// sentence something a caller can branch on. An out parameter documented as
// optional may still be NULL, and then nothing is written to it at all.

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
//     "cacheMaxBytes":    0,       // 0 lets the backend size itself
//     "userAgent":        "...",
//     "resourceDir":      "...",   // where shotium_data.pak and shotium_strings.pak
//                                  //   are; see below
//     "allowFileAccess":  false    // what a request that says nothing means
//   }
//
// cacheMaxBytes is worth setting once caching is on by default. Zero is not
// "unlimited" -- the simple backend picks a fraction of the volume's free
// space -- but it is a number nobody chose, and a cache in a temporary
// directory that sizes itself against a 2 TB disk is a surprise waiting in
// somebody's CI.
//
// resourceDir matters more than it looks. An executable finds its resource
// packs next to itself and a shared library cannot: on Linux the path base
// resolves for "the current module" is /proc/self/exe, which inside a library
// loaded by node names the node binary. A caller that ships the packs beside
// the library must say where they are. Omitting it keeps the executable's
// behaviour, which is right for a host that put everything in one directory.
//
// On failure `*out_engine` is NULL and `*out_error` receives a message the
// caller can print. `out_error` may be NULL if the caller does not want one;
// the status is still returned.
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
//
// `out_stats` is optional -- pass NULL and nothing is written -- and receives
// one JSON object describing what the capture cost and where the bytes came
// from:
//
//   {
//     "requests": 12, "fromCache": 11, "failed": 0, "bytes": 48213,
//     "httpStatus": 200, "finalUrl": "https://example.com/",
//     "timing": {"fetch": 3.1, "render": 24.8, "setup": 2.1,
//                "wait": 10.2, "lifecycle": 4.0, "paint": 0.4,
//                "raster": 6.1, "encode": 2.0, "total": 30.4}
//   }
//
// Timings are milliseconds as doubles. It is delivered on failure too, when
// `out_error` is also set: a capture that timed out having fetched forty
// subresources is one whose statistics are the whole explanation.
//
// Every number in it was already known inside the engine and none of it used
// to leave. That was the difference between "this screenshot got ten times
// slower" being answerable in a minute and being answerable by rebuilding the
// engine with logging in it.
SHOT_EXPORT shot_status shot_engine_capture(shot_engine* engine,
                                            const char* request_json,
                                            shot_buffer** out_image,
                                            shot_buffer** out_stats,
                                            shot_buffer** out_error);

// The tiles of a tiles capture: images in document order, each with where in
// the document it came from. Freed with shot_tile_list_free(), which also
// frees every image not taken out of it first.
typedef struct shot_tile_list shot_tile_list;

// A screenshot in tiles. The request is a ScreenshotOptions with `tile` set:
//
//   {"file":"https://example.com","fullPage":true,"tile":{"height":8000}}
//
// The region -- fullPage, selector, clip or the viewport -- is rendered in
// horizontal slices of at most tile.height CSS pixels, each encoded on its
// own. The document is loaded and laid out once for all of them. It is the
// way to the whole of a page taller than a single image can be; it is not a
// way to spend less memory, because a plain capture already streams its rows.
//
// With `path`, every tile is written to disk and the path must contain `{n}`,
// which becomes the tile's 1-based index; the list then carries the paths and
// empty images.
//
// Everything else -- blocking, serialisation, `out_stats` on success and on
// failure -- is as for shot_engine_capture(). Exactly one of `*out_tiles` and
// `*out_error` is set.
SHOT_EXPORT shot_status shot_engine_capture_tiles(shot_engine* engine,
                                                  const char* request_json,
                                                  shot_tile_list** out_tiles,
                                                  shot_buffer** out_stats,
                                                  shot_buffer** out_error);

SHOT_EXPORT size_t shot_tile_list_count(const shot_tile_list* tiles);

// Where tile `index` came from, in CSS pixels of the document -- the same
// space a request's `clip` is given in. Any of the outputs may be NULL.
SHOT_EXPORT void shot_tile_list_region(const shot_tile_list* tiles,
                                       size_t index,
                                       int32_t* out_x,
                                       int32_t* out_y,
                                       int32_t* out_width,
                                       int32_t* out_height);

// The file tile `index` was written to when the request named a `path`, as a
// NUL-terminated UTF-8 string that lives as long as the list; NULL when the
// bytes were returned instead.
SHOT_EXPORT const char* shot_tile_list_path(const shot_tile_list* tiles,
                                            size_t index);

// Takes tile `index`'s image out of the list. The caller owns it from here and
// frees it with shot_buffer_free(); the list no longer will. NULL for an index
// out of range, or a tile already taken.
SHOT_EXPORT shot_buffer* shot_tile_list_take_image(shot_tile_list* tiles,
                                                   size_t index);

SHOT_EXPORT void shot_tile_list_free(shot_tile_list* tiles);

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

// What the engine came up as.
//
//   {"cacheDir": "..." | null, "cacheActive": true}
//
// cacheActive is the one worth having. A directory that cannot be created or
// written to costs nothing visible -- the engine renders exactly as well
// without a cache, only slower, and every capture pays the network again for
// a reason nothing reports. The engine opens the cache during startup so that
// this is answerable before the first screenshot rather than after it.
//
// false with a directory set means the open failed. false with
// `cacheDir: null` means no cache was asked for.
//
// Note what it does *not* mean: sharing. Several processes may point at one
// directory and all of them cache -- the simple backend takes no lock across
// processes -- so `true` in two engines at once is the ordinary answer and not
// a contradiction.
SHOT_EXPORT shot_status shot_engine_status(shot_engine* engine,
                                           shot_buffer** out_json,
                                           shot_buffer** out_error);

// The HTTP cache, from outside a capture.
//
// `engine` may be NULL, and passing it when there is one is not optional.
// Within a process, disk_cache sequences backends per directory: asking for a
// second one on a directory the engine holds waits for the engine's to go
// away, which it will not do while the engine is up. Given an engine, these
// run on its thread and borrow the backend it already holds; given NULL, they
// build a small task environment of their own, open the directory, and take it
// down again.
//
// `options_json` for both:
//
//   {
//     "cacheDir": "...",        // required
//     "urls":     ["..."],      // clear only: exact resource URLs
//     "unusedSinceMs": 0,       // clear only: Unix epoch milliseconds
//     "maxBytes": 0             // clear only: evict LRU down to this
//   }
//
// The three clear filters compose, and a clear with none of them empties the
// cache. There is deliberately no pattern syntax here -- the caller lists,
// matches with whatever dialect its own users expect, and passes back the URLs
// it decided on.
//
// `*out_json` receives the answer: an array of {url, lastUsedMs, bytes} for
// list, and {removed, bytesBefore, bytesAfter} for clear. `removed` is -1 when
// the whole cache was dropped in one operation, which the backend does without
// counting.
SHOT_EXPORT shot_status shot_cache_list(shot_engine* engine,
                                        const char* options_json,
                                        shot_buffer** out_json,
                                        shot_buffer** out_error);

SHOT_EXPORT shot_status shot_cache_clear(shot_engine* engine,
                                         const char* options_json,
                                         shot_buffer** out_json,
                                         shot_buffer** out_error);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // SHOT_SHOT_API_H_
