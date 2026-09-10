// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Compiled by GN with the engine's allocator and C++ runtime. Node-API is
// the only host ABI; no V8, JSON C seam, or libuv waiting worker is involved.
#include <node_api.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <limits>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/values.h"
#include "shot/shot_engine.h"
#include "shot/shot_request.h"

namespace {
constexpr int kBindingVersion = 1;
struct CacheWorker {
  std::thread thread;
  std::shared_ptr<std::atomic<bool>> done;
};
struct Environment {
  std::shared_ptr<shot::EngineService> engine;
  std::vector<CacheWorker> cache_threads;
  std::mutex mutex;
  bool closing = false;
  napi_threadsafe_function completion = nullptr;
  size_t pending = 0;
};
struct Task {
  napi_deferred deferred = nullptr;
  Environment* owner = nullptr;
  std::shared_ptr<shot::EngineService> engine;
  shot::EngineResult result;
  bool tiles = false;
  bool cache = false;
};

bool Check(napi_env env, napi_status status) {
  if (status == napi_ok) {
    return true;
  }
  bool pending = false;
  napi_is_exception_pending(env, &pending);
  if (!pending) {
    napi_throw_error(env, nullptr, "shotium: Node-API operation failed");
  }
  return false;
}
napi_value Undefined(napi_env env) {
  napi_value value = nullptr;
  Check(env, napi_get_undefined(env, &value));
  return value;
}
napi_value String(napi_env env, const std::string& text) {
  napi_value value = nullptr;
  if (!Check(env,
             napi_create_string_utf8(env, text.data(), text.size(), &value))) {
    return nullptr;
  }
  return value;
}
bool Utf8(napi_env env, napi_value value, std::string* out) {
  size_t size = 0;
  if (!Check(env, napi_get_value_string_utf8(env, value, nullptr, 0, &size))) {
    return false;
  }
  std::string text(size + 1, '\0');
  size_t written = 0;
  if (!Check(env, napi_get_value_string_utf8(env, value, text.data(),
                                             text.size(), &written))) {
    return false;
  }
  text.resize(written);
  *out = std::move(text);
  return true;
}

// Conversion has no field defaults or business rules. The shared dictionary
// readers remain the single native authority for all three entry points.
bool ReadValue(napi_env env,
               napi_value value,
               base::Value* out,
               int depth = 0) {
  if (depth > 16) {
    napi_throw_type_error(env, nullptr,
                          "shotium: options are nested too deeply");
    return false;
  }
  napi_valuetype type;
  if (!Check(env, napi_typeof(env, value, &type))) {
    return false;
  }
  if (type == napi_null || type == napi_undefined) {
    *out = base::Value();
    return true;
  }
  if (type == napi_boolean) {
    bool v = false;
    if (!Check(env, napi_get_value_bool(env, value, &v))) {
      return false;
    }
    *out = base::Value(v);
    return true;
  }
  if (type == napi_number) {
    double v = 0;
    if (!Check(env, napi_get_value_double(env, value, &v))) {
      return false;
    }
    if (!std::isfinite(v)) {
      napi_throw_type_error(env, nullptr, "shotium: numbers must be finite");
      return false;
    }
    if (std::trunc(v) == v && v >= std::numeric_limits<int>::min() &&
        v <= std::numeric_limits<int>::max()) {
      *out = base::Value(static_cast<int>(v));
    } else {
      *out = base::Value(v);
    }
    return true;
  }
  if (type == napi_string) {
    std::string text;
    if (!Utf8(env, value, &text)) {
      return false;
    }
    *out = base::Value(std::move(text));
    return true;
  }
  if (type != napi_object) {
    napi_throw_type_error(env, nullptr,
                          "shotium: options must contain plain values");
    return false;
  }
  bool array = false;
  if (!Check(env, napi_is_array(env, value, &array))) {
    return false;
  }
  napi_value keys = nullptr;
  uint32_t length = 0;
  if (array) {
    if (!Check(env, napi_get_array_length(env, value, &length))) {
      return false;
    }
  } else {
    if (!Check(env, napi_get_all_property_names(
                        env, value, napi_key_own_only,
                        static_cast<napi_key_filter>(napi_key_enumerable |
                                                     napi_key_skip_symbols),
                        napi_key_numbers_to_strings, &keys)) ||
        !Check(env, napi_get_array_length(env, keys, &length))) {
      return false;
    }
  }
  base::ListValue list;
  base::DictValue dict;
  for (uint32_t i = 0; i < length; ++i) {
    napi_value child = nullptr;
    std::string key;
    if (array) {
      if (!Check(env, napi_get_element(env, value, i, &child))) {
        return false;
      }
    } else {
      napi_value name = nullptr;
      if (!Check(env, napi_get_element(env, keys, i, &name)) ||
          !Utf8(env, name, &key) ||
          !Check(env, napi_get_property(env, value, name, &child))) {
        return false;
      }
      napi_valuetype child_type;
      if (!Check(env, napi_typeof(env, child, &child_type))) {
        return false;
      }
      if (child_type == napi_undefined) {
        continue;
      }
    }
    base::Value converted;
    if (!ReadValue(env, child, &converted, depth + 1)) {
      return false;
    }
    if (array) {
      list.Append(std::move(converted));
    } else {
      dict.Set(key, std::move(converted));
    }
  }
  *out = array ? base::Value(std::move(list)) : base::Value(std::move(dict));
  return true;
}
bool ReadDict(napi_env env, napi_value value, base::DictValue* out) {
  base::Value converted;
  if (!ReadValue(env, value, &converted)) {
    return false;
  }
  if (!converted.is_dict()) {
    napi_throw_type_error(env, nullptr, "shotium: options must be an object");
    return false;
  }
  *out = std::move(converted).TakeDict();
  return true;
}

napi_value WriteValue(napi_env env, const base::Value& value) {
  napi_value out = nullptr;
  if (value.is_none()) {
    if (!Check(env, napi_get_null(env, &out))) {
      return nullptr;
    }
  } else if (value.is_bool()) {
    if (!Check(env, napi_get_boolean(env, value.GetBool(), &out))) {
      return nullptr;
    }
  } else if (value.is_int()) {
    if (!Check(env, napi_create_int32(env, value.GetInt(), &out))) {
      return nullptr;
    }
  } else if (value.is_double()) {
    if (!Check(env, napi_create_double(env, value.GetDouble(), &out))) {
      return nullptr;
    }
  } else if (value.is_string()) {
    return String(env, value.GetString());
  } else if (value.is_list()) {
    if (!Check(env, napi_create_array_with_length(env, value.GetList().size(),
                                                  &out))) {
      return nullptr;
    }
    uint32_t i = 0;
    for (const auto& child : value.GetList()) {
      auto item = WriteValue(env, child);
      if (!item || !Check(env, napi_set_element(env, out, i++, item))) {
        return nullptr;
      }
    }
  } else if (value.is_dict()) {
    if (!Check(env, napi_create_object(env, &out))) {
      return nullptr;
    }
    for (const auto [key, child] : value.GetDict()) {
      auto item = WriteValue(env, child);
      if (!item ||
          !Check(env, napi_set_named_property(env, out, key.c_str(), item))) {
        return nullptr;
      }
    }
  }
  return out;
}
bool Set(napi_env env, napi_value object, const char* name, napi_value value) {
  return value && Check(env, napi_set_named_property(env, object, name, value));
}
void FreeImage(napi_env, void*, void* hint) {
  delete static_cast<shot::Bytes*>(hint);
}
napi_value Image(napi_env env, shot::Bytes bytes) {
  napi_value value = nullptr;
  auto owned = std::make_unique<shot::Bytes>(std::move(bytes));
  if (owned->empty()) {
    if (!Check(env, napi_create_buffer(env, 0, nullptr, &value))) {
      return nullptr;
    }
    return value;
  }
  auto status = napi_create_external_buffer(env, owned->size(),
                                            const_cast<uint8_t*>(owned->data()),
                                            FreeImage, owned.get(), &value);
  if (status == napi_ok) {
    owned.release();
    return value;
  }
  if (status != napi_no_external_buffers_allowed) {
    Check(env, status);
    return nullptr;
  }
  if (!Check(env, napi_create_buffer_copy(env, owned->size(), owned->data(),
                                          nullptr, &value))) {
    return nullptr;
  }
  return value;
}

napi_value Result(napi_env env, Task* task) {
  auto& result = task->result;
  napi_value out = nullptr;
  if (result.status != shot::EngineStatus::kOk) {
    auto message = String(env, result.error);
    if (!message) {
      return nullptr;
    }
    // Preserve the old adapter's Error class for shared engine validation.
    auto status = napi_create_error(env, nullptr, message, &out);
    if (!Check(env, status)) {
      return nullptr;
    }
  } else if (task->cache) {
    return WriteValue(env, result.value);
  } else {
    if (!Check(env, napi_create_object(env, &out))) {
      return nullptr;
    }
    if (!task->tiles) {
      if (!Set(env, out, "image", Image(env, std::move(result.image)))) {
        return nullptr;
      }
    } else {
      napi_value list = nullptr;
      if (!Check(env, napi_create_array_with_length(env, result.tiles.size(),
                                                    &list))) {
        return nullptr;
      }
      uint32_t i = 0;
      for (auto& tile : result.tiles) {
        base::DictValue metadata;
        metadata.Set("x", tile.region.x());
        metadata.Set("y", tile.region.y());
        metadata.Set("width", tile.region.width());
        metadata.Set("height", tile.region.height());
        if (!tile.path.empty()) {
          metadata.Set("path", tile.path);
        }
        auto item = WriteValue(env, base::Value(std::move(metadata)));
        if (!item ||
            !Set(env, item, "image", Image(env, std::move(tile.image))) ||
            !Check(env, napi_set_element(env, list, i++, item))) {
          return nullptr;
        }
      }
      if (!Set(env, out, "tiles", list)) {
        return nullptr;
      }
    }
  }
  if (result.stats &&
      !Set(env, out, "stats",
           WriteValue(env, base::Value(shot::StatsToValue(*result.stats))))) {
    return nullptr;
  }
  return out;
}

void Complete(napi_env env, napi_value, void*, void* data) {
  std::unique_ptr<Task> task(static_cast<Task*>(data));
  // During teardown Node calls this with no env: only native resources may
  // be touched. Buffer ownership has not been transferred in that case.
  if (!env || !task) {
    return;
  }
  auto* owner = task->owner;
  if (--owner->pending == 0) {
    napi_unref_threadsafe_function(env, owner->completion);
  }
  auto value = Result(env, task.get());
  bool failed = task->result.status != shot::EngineStatus::kOk;
  if (!value) {
    bool pending = false;
    napi_is_exception_pending(env, &pending);
    if (pending) {
      napi_get_and_clear_last_exception(env, &value);
    }
    if (!value) {
      auto message = String(env, "shotium: could not construct the result");
      if (message) {
        napi_create_error(env, nullptr, message, &value);
      }
    }
    failed = true;
  }
  if (value) {
    if (failed) {
      napi_reject_deferred(env, task->deferred, value);
    } else {
      napi_resolve_deferred(env, task->deferred, value);
    }
  }
}
void Deliver(Task* task, shot::EngineResult result) {
  task->result = std::move(result);
  auto* state = task->owner;
  std::lock_guard lock(state->mutex);
  if (state->closing) {
    delete task;
    return;
  }
  // The environment's cleanup hook is registered AFTER this shared TSFN's
  // hook. It takes this mutex before aborting the TSFN, so a renderer can
  // never notify through a handle Node has already freed during termination.
  auto status = napi_call_threadsafe_function(state->completion, task,
                                              napi_tsfn_nonblocking);
  if (status != napi_ok) {
    delete task;
  }
}
Task* NewTask(napi_env env, Environment* owner, napi_value* promise) {
  auto task = std::make_unique<Task>();
  if (!Check(env, napi_create_promise(env, &task->deferred, promise))) {
    return nullptr;
  }
  if (owner->pending == 0 &&
      !Check(env, napi_ref_threadsafe_function(env, owner->completion))) {
    return nullptr;
  }
  ++owner->pending;
  task->owner = owner;
  return task.release();
}
Environment* Args(napi_env env,
                  napi_callback_info info,
                  size_t* argc,
                  napi_value* argv) {
  void* data = nullptr;
  if (!Check(env, napi_get_cb_info(env, info, argc, argv, nullptr, &data))) {
    return nullptr;
  }
  auto* state = static_cast<Environment*>(data);
  if (state->closing) {
    napi_throw_error(env, nullptr, "shotium: environment is closing");
    return nullptr;
  }
  return state;
}
bool Handle(napi_env env,
            napi_value value,
            Environment* state,
            bool nullable = false) {
  napi_valuetype type;
  if (!Check(env, napi_typeof(env, value, &type))) {
    return false;
  }
  if (nullable && type == napi_null) {
    return true;
  }
  void* pointer = nullptr;
  if (type != napi_external ||
      napi_get_value_external(env, value, &pointer) != napi_ok ||
      pointer != state) {
    napi_throw_type_error(env, nullptr, "shotium: expected an engine handle");
    return false;
  }
  if (!state->engine) {
    napi_throw_error(env, nullptr, "shotium: this engine has been destroyed");
    return false;
  }
  return true;
}
napi_value Create(napi_env env, napi_callback_info info) {
  size_t argc = 1;
  napi_value argv[1] = {};
  auto* state = Args(env, info, &argc, argv);
  if (!state) {
    return nullptr;
  }
  base::DictValue dict;
  if (argc && !ReadDict(env, argv[0], &dict)) {
    return nullptr;
  }
  auto options = shot::ReadEngineOptions(dict);
  if (!options.has_value()) {
    napi_throw_error(env, nullptr, options.error().c_str());
    return nullptr;
  }
  auto engine = shot::EngineService::Create(std::move(*options));
  if (!engine.has_value()) {
    napi_throw_error(env, nullptr, engine.error().c_str());
    return nullptr;
  }
  state->engine = std::move(*engine);
  napi_value handle = nullptr;
  if (!Check(env,
             napi_create_external(env, state, nullptr, nullptr, &handle))) {
    state->engine->Stop();
    state->engine.reset();
    return nullptr;
  }
  return handle;
}
napi_value Capture(napi_env env, napi_callback_info info, bool tiles) {
  size_t argc = 2;
  napi_value argv[2] = {};
  auto* state = Args(env, info, &argc, argv);
  if (!state) {
    return nullptr;
  }
  if (argc < 2) {
    napi_throw_type_error(env, nullptr,
                          "shotium: capture needs a handle and request");
    return nullptr;
  }
  base::DictValue dict;
  if (!ReadDict(env, argv[1], &dict)) {
    return nullptr;
  }
  // Reading getters can reenter this binding and destroy the engine.
  if (!Handle(env, argv[0], state)) {
    return nullptr;
  }
  auto request = shot::ReadScreenshotRequest(
      dict, state->engine->default_allow_file_access());
  napi_value promise = nullptr;
  Task* task = NewTask(env, state, &promise);
  if (!task) {
    return nullptr;
  }
  task->engine = state->engine;
  task->tiles = tiles;
  if (!request.has_value()) {
    shot::EngineResult failure;
    failure.status = shot::EngineStatus::kUsage;
    failure.error = request.error();
    Deliver(task, std::move(failure));
  } else if (!state->engine->Capture(
                 std::move(*request), tiles,
                 base::BindOnce(Deliver, base::Unretained(task)))) {
    shot::EngineResult failure;
    failure.status = shot::EngineStatus::kState;
    failure.error = "shotium: engine is closing";
    Deliver(task, std::move(failure));
  }
  return promise;
}
napi_value CaptureOne(napi_env env, napi_callback_info info) {
  return Capture(env, info, false);
}
napi_value CaptureTiles(napi_env env, napi_callback_info info) {
  return Capture(env, info, true);
}
napi_value Cache(napi_env env, napi_callback_info info) {
  size_t argc = 3;
  napi_value argv[3] = {};
  auto* state = Args(env, info, &argc, argv);
  if (!state) {
    return nullptr;
  }
  if (argc < 3) {
    napi_throw_type_error(env, nullptr,
                          "shotium: cache needs a handle, mode and options");
    return nullptr;
  }
  if (!Handle(env, argv[0], state, true)) {
    return nullptr;
  }
  bool clearing = false;
  if (!Check(env, napi_get_value_bool(env, argv[1], &clearing))) {
    return nullptr;
  }
  base::DictValue dict;
  if (!ReadDict(env, argv[2], &dict)) {
    return nullptr;
  }
  auto options = shot::ReadCacheOptions(dict);
  if (!options.has_value()) {
    napi_throw_error(env, nullptr, options.error().c_str());
    return nullptr;
  }
  napi_value promise = nullptr;
  Task* task = NewTask(env, state, &promise);
  if (!task) {
    return nullptr;
  }
  task->cache = true;
  // Even a null handle must borrow the engine if this environment owns one.
  task->engine = state->engine;
  if (task->engine) {
    if (!task->engine->Cache(std::move(*options), clearing,
                             base::BindOnce(Deliver, base::Unretained(task)))) {
      shot::EngineResult failure;
      failure.status = shot::EngineStatus::kState;
      failure.error = "shotium: engine is closing";
      Deliver(task, std::move(failure));
    }
  } else {
    std::erase_if(state->cache_threads, [](CacheWorker& worker) {
      if (!worker.done->load()) {
        return false;
      }
      worker.thread.join();
      return true;
    });
    auto done = std::make_shared<std::atomic<bool>>(false);
    std::thread thread(
        [task, options = std::move(*options), clearing, done]() mutable {
          Deliver(task, shot::CacheWithoutEngine(std::move(options), clearing));
          done->store(true);
        });
    state->cache_threads.push_back({std::move(thread), std::move(done)});
  }
  return promise;
}
napi_value Manage(napi_env env, napi_callback_info info, int action) {
  size_t argc = 2;
  napi_value argv[2] = {};
  auto* state = Args(env, info, &argc, argv);
  if (!state) {
    return nullptr;
  }
  if (!argc) {
    napi_throw_type_error(env, nullptr, "shotium: expected an engine handle");
    return nullptr;
  }
  if (!Handle(env, argv[0], state)) {
    return nullptr;
  }
  if (action == 0) {
    return WriteValue(env, base::Value(state->engine->Status()));
  }
  if (action == 1) {
    bool release = false;
    if (argc > 1 && !Check(env, napi_get_value_bool(env, argv[1], &release))) {
      return nullptr;
    }
    state->engine->Purge(release);
  } else {
    state->engine->Stop();
    state->engine.reset();
  }
  return Undefined(env);
}
napi_value Status(napi_env env, napi_callback_info info) {
  return Manage(env, info, 0);
}
napi_value Purge(napi_env env, napi_callback_info info) {
  return Manage(env, info, 1);
}
napi_value Destroy(napi_env env, napi_callback_info info) {
  return Manage(env, info, 2);
}

void Cleanup(napi_async_cleanup_hook_handle hook, void* data) {
  auto* state = static_cast<Environment*>(data);
  {
    std::lock_guard lock(state->mutex);
    state->closing = true;
    napi_release_threadsafe_function(state->completion, napi_tsfn_abort);
  }
  // Joining cannot require the JS loop: engine completions only enqueue
  // nonblocking notifications. The async hook keeps state alive until joined.
  std::thread([state, hook]() {
    for (auto& worker : state->cache_threads) {
      worker.thread.join();
    }
    if (state->engine) {
      state->engine->Stop();
    }
    state->engine.reset();
    napi_remove_async_cleanup_hook(hook);
  }).detach();
}
void FinalizeEnvironment(napi_env, void* data, void*) {
  delete static_cast<Environment*>(data);
}
napi_value Init(napi_env env, napi_value exports) {
  auto state = std::make_unique<Environment>();
  napi_property_descriptor properties[] = {
      {"create", nullptr, Create, nullptr, nullptr, nullptr, napi_default,
       state.get()},
      {"capture", nullptr, CaptureOne, nullptr, nullptr, nullptr, napi_default,
       state.get()},
      {"captureTiles", nullptr, CaptureTiles, nullptr, nullptr, nullptr,
       napi_default, state.get()},
      {"cache", nullptr, Cache, nullptr, nullptr, nullptr, napi_default,
       state.get()},
      {"status", nullptr, Status, nullptr, nullptr, nullptr, napi_default,
       state.get()},
      {"purge", nullptr, Purge, nullptr, nullptr, nullptr, napi_default,
       state.get()},
      {"destroy", nullptr, Destroy, nullptr, nullptr, nullptr, napi_default,
       state.get()},
  };
  if (!Check(env, napi_define_properties(env, exports, std::size(properties),
                                         properties))) {
    return nullptr;
  }
  napi_value version = nullptr;
  if (!Check(env, napi_create_int32(env, kBindingVersion, &version)) ||
      !Set(env, exports, "bindingVersion", version)) {
    return nullptr;
  }
  if (!Check(env, napi_set_instance_data(env, state.get(), FinalizeEnvironment,
                                         nullptr))) {
    return nullptr;
  }
  auto* owned = state.release();
  auto name = String(env, "shotium completion");
  if (!name ||
      !Check(env, napi_create_threadsafe_function(
                      env, nullptr, nullptr, name, 0, 1, nullptr, nullptr,
                      nullptr, Complete, &owned->completion))) {
    return nullptr;
  }
  if (!Check(env, napi_unref_threadsafe_function(env, owned->completion))) {
    return nullptr;
  }
  // Must be registered after the TSFN: Node invokes cleanup hooks in reverse
  // order, including when a Worker terminates while rendering is in flight.
  if (!Check(env, napi_add_async_cleanup_hook(env, Cleanup, owned, nullptr))) {
    return nullptr;
  }
  return exports;
}
}  // namespace
NAPI_MODULE(shotium, Init)
