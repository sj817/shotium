// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// shotium's node addon: shot in this process, over shot_api.h.
//
// It is deliberately thin. Everything about what a screenshot means -- which
// fields exist, what they default to, what an unknown one is -- lives in
// shotium/src/lib/request.ts and shot/shot_request.cc, and this file carries
// JSON between them without reading it. Anything it understood would be a
// third opinion about the request format, and the third opinion is always the
// one that drifts.
//
// Node-API rather than V8: the ABI is stable across node versions, so one
// prebuilt .node per platform is enough and the addon does not have to be
// rebuilt every time node's internals move.

#include <node_api.h>

#include <cstring>
#include <string>
#include <utility>

#include "shot_api.h"

namespace {

// A shot_engine, as node sees it.
//
// Wrapped rather than handed over as a bare pointer so that a script dropping
// the handle on the floor still shuts the engine down: the finalizer runs when
// the object is collected. `engine` is cleared by an explicit destroy() so the
// finalizer does not do it twice.
struct EngineHandle {
  shot_engine* engine = nullptr;
};

void FinalizeEngine(napi_env env, void* data, void* hint) {
  auto* handle = static_cast<EngineHandle*>(data);
  if (handle->engine) {
    shot_engine_destroy(handle->engine);
  }
  delete handle;
}

// One capture in flight: a promise, and the strings on both sides of it.
//
// The request is copied rather than referenced because Execute runs on a
// libuv thread where no JS value may be touched, and the JS string it came
// from can be collected before then.
struct CaptureTask {
  napi_deferred deferred = nullptr;
  napi_async_work work = nullptr;
  shot_engine* engine = nullptr;
  std::string request;
  shot_status status = SHOT_ERR_CAPTURE;
  shot_buffer* image = nullptr;
  shot_buffer* stats = nullptr;
  shot_buffer* error = nullptr;
};

// One cache operation in flight. The same shape as a capture and deliberately
// not shared with it: a capture resolves to bytes and this resolves to JSON,
// and the one field they would have in common is the promise.
struct CacheTask {
  napi_deferred deferred = nullptr;
  napi_async_work work = nullptr;
  // Null is meaningful here rather than an error: it says there is no engine
  // in this process, and the library should open the directory itself.
  shot_engine* engine = nullptr;
  bool clearing = false;
  std::string options;
  shot_status status = SHOT_ERR_STATE;
  shot_buffer* json = nullptr;
  shot_buffer* error = nullptr;
};

bool ReadUtf8(napi_env env, napi_value value, std::string* out) {
  size_t length = 0;
  if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
    return false;
  }
  // Room for the NUL node insists on writing, then cut back to what it says
  // it wrote.
  std::string text(length + 1, '\0');
  size_t written = 0;
  if (napi_get_value_string_utf8(env, value, text.data(), length + 1,
                                 &written) != napi_ok) {
    return false;
  }
  text.resize(written);
  *out = std::move(text);
  return true;
}

napi_value Undefined(napi_env env) {
  napi_value value = nullptr;
  napi_get_undefined(env, &value);
  return value;
}

// Turns a shot_buffer carrying a message into a thrown JS error. Freeing it is
// this function's job either way, because every caller is on its way out.
void ThrowFromBuffer(napi_env env, shot_buffer* message, const char* fallback) {
  const char* text = fallback;
  if (message && shot_buffer_size(message) > 0) {
    text = reinterpret_cast<const char*>(shot_buffer_data(message));
  }
  napi_throw_error(env, nullptr, text);
  shot_buffer_free(message);
}

bool ReadHandle(napi_env env, napi_value value, EngineHandle** out) {
  void* data = nullptr;
  if (napi_get_value_external(env, value, &data) != napi_ok || !data) {
    napi_throw_type_error(env, nullptr, "shotium: expected an engine handle");
    return false;
  }
  *out = static_cast<EngineHandle*>(data);
  if (!(*out)->engine) {
    napi_throw_error(env, nullptr, "shotium: this engine has been destroyed");
    return false;
  }
  return true;
}

napi_value Create(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {};
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

  std::string options;
  if (argc > 0 && !ReadUtf8(env, argv[0], &options)) {
    napi_throw_type_error(env, nullptr,
                          "shotium: create(optionsJson) wants a string");
    return nullptr;
  }

  if (shot_abi_version() != SHOT_ABI_VERSION) {
    napi_throw_error(env, nullptr,
                     "shotium: the shot library beside this addon speaks a "
                     "different ABI version; they ship together and one of "
                     "them has been replaced");
    return nullptr;
  }

  shot_engine* engine = nullptr;
  shot_buffer* error = nullptr;
  if (shot_engine_create(options.c_str(), &engine, &error) != SHOT_OK) {
    ThrowFromBuffer(env, error, "shotium: the engine would not start");
    return nullptr;
  }

  auto* handle = new EngineHandle{engine};
  napi_value external = nullptr;
  if (napi_create_external(env, handle, FinalizeEngine, nullptr, &external) !=
      napi_ok) {
    shot_engine_destroy(engine);
    delete handle;
    napi_throw_error(env, nullptr, "shotium: could not wrap the engine");
    return nullptr;
  }
  return external;
}

napi_value Destroy(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {};
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

  void* data = nullptr;
  if (argc < 1 || napi_get_value_external(env, argv[0], &data) != napi_ok ||
      !data) {
    napi_throw_type_error(env, nullptr, "shotium: expected an engine handle");
    return nullptr;
  }
  auto* handle = static_cast<EngineHandle*>(data);
  if (handle->engine) {
    shot_engine_destroy(handle->engine);
    handle->engine = nullptr;
  }
  return Undefined(env);
}

// Runs on a libuv thread. No napi call is legal here beyond the ones that take
// no env, which is why everything it needs was copied out first.
void ExecuteCapture(napi_env env, void* data) {
  auto* task = static_cast<CaptureTask*>(data);
  task->status =
      shot_engine_capture(task->engine, task->request.c_str(), &task->image,
                          &task->stats, &task->error);
}

// The statistics buffer as a JS string, or undefined when there is none.
//
// Left as a string rather than parsed here: this file carries JSON between the
// engine and the JS layer without reading it, and a JSON.parse on this side
// would be a second opinion about the shape -- the thing the comment at the
// top of this file exists to rule out.
napi_value StatsValue(napi_env env, shot_buffer* stats) {
  if (!stats || shot_buffer_size(stats) == 0) {
    return Undefined(env);
  }
  napi_value value = nullptr;
  napi_create_string_utf8(env,
                          reinterpret_cast<const char*>(shot_buffer_data(stats)),
                          shot_buffer_size(stats), &value);
  return value;
}

void CompleteCapture(napi_env env, napi_status status, void* data) {
  auto* task = static_cast<CaptureTask*>(data);

  if (status == napi_ok && task->status == SHOT_OK) {
    // Copied into a node Buffer rather than handed over as external memory.
    // An external buffer would save a memcpy of a few hundred kilobytes
    // against a render that took tens of milliseconds, and would put the
    // lifetime of shot's allocation in the hands of node's GC -- across an
    // allocator boundary the whole C ABI exists to keep closed.
    napi_value buffer = nullptr;
    napi_create_buffer_copy(env, shot_buffer_size(task->image),
                            shot_buffer_data(task->image), nullptr, &buffer);

    // An object rather than the buffer alone, because there are now two
    // answers and a caller that wanted only the first should still not have to
    // ask for a second call to get the other.
    napi_value result = nullptr;
    napi_create_object(env, &result);
    napi_set_named_property(env, result, "image", buffer);
    napi_set_named_property(env, result, "stats",
                            StatsValue(env, task->stats));
    napi_resolve_deferred(env, task->deferred, result);
  } else {
    const char* text = "shotium: the capture failed";
    if (task->error && shot_buffer_size(task->error) > 0) {
      text = reinterpret_cast<const char*>(shot_buffer_data(task->error));
    }
    napi_value message = nullptr;
    napi_value error_value = nullptr;
    napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &message);
    napi_create_error(env, nullptr, message, &error_value);
    // A capture that failed part of the way through still measured what it
    // did, and that is usually the explanation -- forty subresources fetched
    // and the one that mattered timed out. Attached to the error because a
    // rejection has nowhere else to carry it.
    napi_set_named_property(env, error_value, "stats",
                            StatsValue(env, task->stats));
    napi_reject_deferred(env, task->deferred, error_value);
  }

  shot_buffer_free(task->image);
  shot_buffer_free(task->stats);
  shot_buffer_free(task->error);
  napi_delete_async_work(env, task->work);
  delete task;
}

void ExecuteCache(napi_env env, void* data) {
  auto* task = static_cast<CacheTask*>(data);
  task->status = task->clearing
                     ? shot_cache_clear(task->engine, task->options.c_str(),
                                        &task->json, &task->error)
                     : shot_cache_list(task->engine, task->options.c_str(),
                                       &task->json, &task->error);
}

void CompleteCache(napi_env env, napi_status status, void* data) {
  auto* task = static_cast<CacheTask*>(data);

  if (status == napi_ok && task->status == SHOT_OK) {
    napi_value json = nullptr;
    napi_create_string_utf8(
        env, reinterpret_cast<const char*>(shot_buffer_data(task->json)),
        shot_buffer_size(task->json), &json);
    napi_resolve_deferred(env, task->deferred, json);
  } else {
    const char* text = "shotium: the cache operation failed";
    if (task->error && shot_buffer_size(task->error) > 0) {
      text = reinterpret_cast<const char*>(shot_buffer_data(task->error));
    }
    napi_value message = nullptr;
    napi_value error_value = nullptr;
    napi_create_string_utf8(env, text, NAPI_AUTO_LENGTH, &message);
    napi_create_error(env, nullptr, message, &error_value);
    napi_reject_deferred(env, task->deferred, error_value);
  }

  shot_buffer_free(task->json);
  shot_buffer_free(task->error);
  napi_delete_async_work(env, task->work);
  delete task;
}

napi_value Capture(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[2] = {};
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

  EngineHandle* handle = nullptr;
  if (argc < 2 || !ReadHandle(env, argv[0], &handle)) {
    return nullptr;
  }

  auto* task = new CaptureTask;
  task->engine = handle->engine;
  if (!ReadUtf8(env, argv[1], &task->request)) {
    delete task;
    napi_throw_type_error(env, nullptr,
                          "shotium: capture(engine, requestJson) wants a "
                          "string");
    return nullptr;
  }

  napi_value promise = nullptr;
  if (napi_create_promise(env, &task->deferred, &promise) != napi_ok) {
    delete task;
    napi_throw_error(env, nullptr, "shotium: could not make a promise");
    return nullptr;
  }

  napi_value name = nullptr;
  napi_create_string_utf8(env, "shot:capture", NAPI_AUTO_LENGTH, &name);
  napi_create_async_work(env, nullptr, name, ExecuteCapture, CompleteCapture,
                         task, &task->work);
  napi_queue_async_work(env, task->work);
  return promise;
}

// status(engine) -> string
//
// Synchronous, like purge: it reads two fields the engine already knows and
// the caller is holding a promise open for nothing if it waits on the event
// loop for them.
napi_value Status(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {};
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

  EngineHandle* handle = nullptr;
  if (argc < 1 || !ReadHandle(env, argv[0], &handle)) {
    return nullptr;
  }

  shot_buffer* json = nullptr;
  shot_buffer* error = nullptr;
  if (shot_engine_status(handle->engine, &json, &error) != SHOT_OK) {
    ThrowFromBuffer(env, error, "shotium: could not read the engine's status");
    return nullptr;
  }

  napi_value value = nullptr;
  napi_create_string_utf8(env,
                          reinterpret_cast<const char*>(shot_buffer_data(json)),
                          shot_buffer_size(json), &value);
  shot_buffer_free(json);
  shot_buffer_free(error);
  return value;
}

// cache(engineOrNull, clearing, optionsJson) -> Promise<string>
//
// The engine argument is nullable and that is the whole interface: with one,
// the operation runs on the engine's thread and borrows the backend it already
// holds; without one, the library builds a small environment and opens the
// directory itself. The JS layer knows which it has and this does not have to
// guess.
napi_value Cache(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value argv[3] = {};
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

  if (argc < 3) {
    napi_throw_type_error(
        env, nullptr, "shotium: cache(engine, clearing, optionsJson) wants "
                      "three arguments");
    return nullptr;
  }

  auto* task = new CacheTask;

  // Null and undefined both mean "no engine". Anything else has to be a
  // handle, and a handle that has been destroyed is an error rather than a
  // silent fallback to opening the directory -- the directory may still be
  // locked by an engine that is on its way down.
  napi_valuetype type = napi_undefined;
  napi_typeof(env, argv[0], &type);
  if (type != napi_null && type != napi_undefined) {
    EngineHandle* handle = nullptr;
    if (!ReadHandle(env, argv[0], &handle)) {
      delete task;
      return nullptr;
    }
    task->engine = handle->engine;
  }

  napi_get_value_bool(env, argv[1], &task->clearing);
  if (!ReadUtf8(env, argv[2], &task->options)) {
    delete task;
    napi_throw_type_error(env, nullptr,
                          "shotium: cache() wants an options string");
    return nullptr;
  }

  napi_value promise = nullptr;
  if (napi_create_promise(env, &task->deferred, &promise) != napi_ok) {
    delete task;
    napi_throw_error(env, nullptr, "shotium: could not make a promise");
    return nullptr;
  }

  napi_value name = nullptr;
  napi_create_string_utf8(env, "shot:cache", NAPI_AUTO_LENGTH, &name);
  napi_create_async_work(env, nullptr, name, ExecuteCache, CompleteCache, task,
                         &task->work);
  napi_queue_async_work(env, task->work);
  return promise;
}

// Synchronous on purpose. A purge is milliseconds and happens when the caller
// has decided it has nothing else to do; queuing it behind the event loop
// would mean the process that just went idle stays large until something wakes
// it up.
napi_value Purge(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[2] = {};
  napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr);

  EngineHandle* handle = nullptr;
  if (argc < 1 || !ReadHandle(env, argv[0], &handle)) {
    return nullptr;
  }

  bool release = false;
  if (argc > 1) {
    napi_get_value_bool(env, argv[1], &release);
  }
  shot_engine_purge(handle->engine, release ? 1 : 0);
  return Undefined(env);
}

napi_value Init(napi_env env, napi_value exports) {
  const napi_property_descriptor properties[] = {
      {"create", nullptr, Create, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"destroy", nullptr, Destroy, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"capture", nullptr, Capture, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"purge", nullptr, Purge, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"cache", nullptr, Cache, nullptr, nullptr, nullptr, napi_default,
       nullptr},
      {"status", nullptr, Status, nullptr, nullptr, nullptr, napi_default,
       nullptr},
  };
  napi_define_properties(env, exports,
                         sizeof(properties) / sizeof(properties[0]),
                         properties);
  return exports;
}

}  // namespace

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
