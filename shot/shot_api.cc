// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define SHOT_IMPLEMENTATION
#include "shot/shot_api.h"

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/at_exit.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/simple_thread.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/disk_cache/disk_cache.h"
#include "shot/shot_cache.h"
#include "shot/shot_capture.h"
#include "shot/shot_capture_context.h"
#include "shot/shot_bytes.h"
#include "shot/shot_network.h"
#include "shot/shot_request.h"
#include "shot/shot_runtime.h"

// Bytes the library owns.
//
// Declared at namespace scope and not in shot::, because the C header names
// this type and a C compiler has no namespaces to find it in.
struct shot_buffer {
  // May carry a trailing NUL that `size` does not count, so that an error
  // reads as a C string and an image reads as its exact length.
  shot::Bytes bytes;
  size_t size = 0;
};

namespace shot {
namespace {

shot_buffer* MakeImage(shot::Bytes bytes) {
  auto* buffer = new shot_buffer;
  buffer->size = bytes.size();
  buffer->bytes = std::move(bytes);
  return buffer;
}

shot_buffer* MakeMessage(std::string_view text) {
  auto* buffer = new shot_buffer;
  std::vector<uint8_t> bytes(text.begin(), text.end());
  bytes.push_back(0);
  buffer->size = text.size();
  buffer->bytes = shot::Bytes::FromVector(std::move(bytes));
  return buffer;
}

// Hands `text` to an out-parameter the caller may not have wanted. Every
// failure path goes through here so that "the caller passed NULL for the
// error" is answered in one place rather than guarded at each return.
void Deliver(shot_buffer** out, std::string_view text) {
  if (out) {
    *out = MakeMessage(text);
  }
}

// Empties an out-parameter before anything can fill it.
//
// The header promises that exactly one of the answer and the error is set,
// and that promise is only keepable if the ones that are not set are cleared:
// a caller who reuses a variable across calls would otherwise read the
// previous call's buffer back out of it and free it twice. Every entry point
// clears all of its outputs first, so the promise holds from the first line
// rather than from whichever return the call happened to take.
template <typename T>
void Clear(T** out) {
  if (out) {
    *out = nullptr;
  }
}

// The same for an output that is a number rather than something owned. There
// is nothing to double-free here, but a caller who asks for a tile that is not
// there and reads the coordinates anyway should read zeroes rather than
// whatever was in the variable before -- which is the difference between a
// wrong answer and last call's answer.
void Clear(int32_t* out) {
  if (out) {
    *out = 0;
  }
}

// CaptureStats as a JSON string, for the buffer the C ABI hands back.
//
// The object itself is built by StatsToValue, which the resident worker also
// uses; this is only the serialisation the shared library's seam needs.
std::string StatsToJson(const CaptureStats& stats) {
  std::string json;
  base::JSONWriter::Write(StatsToValue(stats), &json);
  return json;
}

// What shot_engine_create() was given, once it has been read.
struct EngineOptions {
  NetworkConfig network;
  base::FilePath resource_dir;
  bool allow_file_access = false;
};

base::expected<EngineOptions, std::string> ParseEngineOptions(
    const char* options_json) {
  EngineOptions options;
  // No options at all is a legitimate thing to ask for, and a caller with
  // nothing to say should not have to spell "{}".
  if (!options_json || *options_json == '\0') {
    return options;
  }

  // JSON_PARSE_RFC, matching shot_request.cc: the caller of a library is
  // usually a serialiser rather than a person, and accepting the extensions
  // (trailing commas, comments) would let one side write what the other side
  // cannot read.
  std::optional<base::DictValue> parsed =
      base::JSONReader::ReadDict(options_json, base::JSON_PARSE_RFC);
  if (!parsed) {
    return base::unexpected("options must be a JSON object");
  }
  const base::DictValue& dict = *parsed;

  // Every field is optional, and a field of the wrong type is rejected rather
  // than ignored: an engine that silently started without the cache the caller
  // asked for is a performance bug nobody can see.
  for (const auto [key, value] : dict) {
    if (key == "allowFileAccess") {
      if (!value.is_bool()) {
        return base::unexpected("allowFileAccess must be a boolean");
      }
      options.allow_file_access = value.GetBool();
    } else if (key == "cacheMaxBytes") {
      // An int and not a double, and checked for sign: the backend takes an
      // int64 where a negative value is not rejected but silently produces a
      // cache that evicts everything it writes.
      if (!value.is_int()) {
        return base::unexpected("cacheMaxBytes must be a number");
      }
      if (value.GetInt() < 0) {
        return base::unexpected("cacheMaxBytes cannot be negative");
      }
      options.network.cache_max_bytes = value.GetInt();
    } else if (key == "cacheDir" || key == "userAgent" ||
               key == "resourceDir") {
      if (!value.is_string()) {
        return base::unexpected(key + " must be a string");
      }
      const std::string& text = value.GetString();
      if (key == "cacheDir") {
        options.network.cache_dir = base::FilePath::FromUTF8Unsafe(text);
      } else if (key == "userAgent") {
        options.network.user_agent = text;
      } else {
        options.resource_dir = base::FilePath::FromUTF8Unsafe(text);
      }
    } else {
      return base::unexpected("unknown option \"" + key + "\"");
    }
  }
  return options;
}

// The thread blink lives on, and the only thread that touches it.
//
// This is what the executable gets from main() for free and a library has to
// build for itself: a thread that belongs to shot, with a task environment on
// it, whose lifetime does not depend on what the host is doing with its own
// threads. Every entry point below is a post onto it and a wait.
class EngineThread : public base::DelegateSimpleThread::Delegate {
 public:
  EngineThread() = default;

  EngineThread(const EngineThread&) = delete;
  EngineThread& operator=(const EngineThread&) = delete;

  // Blocks until the runtime is up, or until it is known not to be coming.
  bool Start(EngineOptions options, std::string* error) {
    options_ = std::move(options);
    thread_ = std::make_unique<base::DelegateSimpleThread>(this, "ShotEngine");
    thread_->Start();
    ready_.Wait();
    if (!error_.empty()) {
      thread_->Join();
      thread_.reset();
      *error = error_;
      return false;
    }
    return true;
  }

  void Stop() {
    if (!thread_) {
      return;
    }
    // QuitClosure is safe from any thread; the loop stops after whatever it is
    // running finishes, which is why a capture in flight is waited for rather
    // than cut off.
    quit_.Run();
    thread_->Join();
    thread_.reset();
  }

  // Runs `task` on the engine thread and waits for it.
  //
  // Callers from several threads at once are fine and are not parallel: the
  // task runner serialises them, which is the only correct answer when the
  // thing they all reach is one blink.
  void RunSync(base::OnceClosure task) {
    base::WaitableEvent done;
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            [](base::OnceClosure task, base::WaitableEvent* done) {
              std::move(task).Run();
              done->Signal();
            },
            std::move(task), base::Unretained(&done)));
    done.Wait();
  }

  bool default_allow_file_access() const { return options_.allow_file_access; }
  ShotRuntime& runtime() { return *runtime_; }

 private:
  // base::DelegateSimpleThread::Delegate:
  void Run() override {
    // The three things main() does before anything else, done here for the
    // same reasons -- and here rather than in shot_engine_create() because
    // AtExitManager's registrations have to be unwound on the thread that
    // built the singletons, which is this one.
    base::AtExitManager at_exit;
    // No argv to pass on: a library was not started from a command line, and
    // taking the host's would mean reading node's flags as shot's.
    base::CommandLine::Init(0, nullptr);

    // stderr, matching the executable. A library writing to the host's stdout
    // would be worse than noise -- for shotium's addon that stream belongs to
    // whatever the host program is saying.
    logging::LoggingSettings log_settings;
    log_settings.logging_dest = logging::LOG_TO_STDERR;
    logging::InitLogging(log_settings);
    logging::SetMinLogLevel(logging::LOGGING_WARNING);

    // Where shotium_data.pak and shotium_strings.pak are. See the header: the
    // executable finds them next to itself through DIR_MODULE and a shared
    // library cannot, because on Linux that path resolves through
    // /proc/self/exe and names the host binary. Overriding the key rather than
    // teaching ShotRuntime a second way to find its resources keeps the
    // difference in the one place it exists.
    if (!options_.resource_dir.empty()) {
      base::PathService::Override(base::DIR_MODULE, options_.resource_dir);
    }

    auto runtime = ShotRuntime::Create(options_.network);
    if (!runtime.has_value()) {
      error_ = runtime.error();
      ready_.Signal();
      return;
    }
    runtime_ = runtime->get();

    {
      base::RunLoop run_loop;
      quit_ = run_loop.QuitClosure();
      task_runner_ = base::SingleThreadTaskRunner::GetCurrentDefault();
      // Everything above is what the waiting caller is waiting to see. The
      // signal is the release side of it: nothing here is read by another
      // thread before the wait returns.
      ready_.Signal();
      run_loop.Run();
    }

    // Both on this thread and in this order: the run loop is the task
    // environment the runtime's teardown posts into, so it outlives nothing
    // and the runtime outlives it by exactly one scope.
    runtime_ = nullptr;
  }

  EngineOptions options_;
  std::unique_ptr<base::DelegateSimpleThread> thread_;
  base::WaitableEvent ready_;
  // Written on the engine thread before ready_, read after it.
  std::string error_;
  raw_ptr<ShotRuntime> runtime_ = nullptr;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::RepeatingClosure quit_;
};

// Blink is a process-wide singleton and its initialisation has no undo, so
// this is not "one at a time" -- it is one, ever. Recording the attempt rather
// than the liveness is deliberate: a caller who destroys an engine and builds
// another would get a process that looks fine and renders wrongly.
std::atomic<bool>& EngineWasCreated() {
  static std::atomic<bool> created(false);
  return created;
}

// A thread with just enough environment on it to run the disk cache, for a
// caller who has no engine.
//
// The disk cache needs three things that a bare thread does not have: an
// AtExitManager for the singletons it builds, a thread pool for its file I/O,
// and a message pump a nested run loop can turn. That is a small fraction of
// what EngineThread sets up -- no blink, no mojo, no resource bundle, no
// network stack -- and it exists separately because the whole point is to work
// when there is no engine to borrow.
//
// It must not be used when there *is* an engine: AtExitManager and
// ThreadPoolInstance are process-wide singletons and the engine's thread holds
// both. shot_cache_* dispatches on that, and this is only reached for a null
// engine.
class CacheThread : public base::DelegateSimpleThread::Delegate {
 public:
  explicit CacheThread(base::OnceClosure task) : task_(std::move(task)) {}

  CacheThread(const CacheThread&) = delete;
  CacheThread& operator=(const CacheThread&) = delete;

  // Builds the environment, runs the task, tears it down, and returns when all
  // of that is finished. Synchronous because every caller is a C entry point
  // that has to have an answer before it returns.
  void RunAndJoin() {
    base::DelegateSimpleThread thread(this, "ShotCache");
    thread.Start();
    thread.Join();
  }

 private:
  // base::DelegateSimpleThread::Delegate:
  void Run() override {
    base::AtExitManager at_exit;
    // Guarded because a process that has already had an engine has already
    // done this and the second call would be the bug rather than the fix.
    if (!base::CommandLine::InitializedForCurrentProcess()) {
      base::CommandLine::Init(0, nullptr);
    }

    const bool owns_thread_pool = !base::ThreadPoolInstance::Get();
    if (owns_thread_pool) {
      // Three, matching ShotRuntime. The simple backend's work is file I/O
      // that blocks rather than computes, and the operations here are issued
      // one at a time anyway.
      base::ThreadPoolInstance::Create("ShotCache");
      base::ThreadPoolInstance::Get()->Start({3});
    }

    {
      // IO, because the cache watches files through the same mechanism //net
      // watches sockets, and because the nested run loops in shot_cache.cc
      // need a pump to turn at all.
      base::SingleThreadTaskExecutor executor(base::MessagePumpType::IO);
      std::move(task_).Run();
    }

    if (owns_thread_pool) {
      base::ThreadPoolInstance::Get()->Shutdown();
    }
  }

  base::OnceClosure task_;
};

// The cacheDir every shot_cache_* call needs, and the optional filters only
// the clear takes.
struct CacheOptions {
  base::FilePath directory;
  CacheClearOptions clear;
};

base::expected<CacheOptions, std::string> ParseCacheOptions(
    const char* options_json) {
  if (!options_json || *options_json == '\0') {
    return base::unexpected("cacheDir is required");
  }
  std::optional<base::DictValue> parsed =
      base::JSONReader::ReadDict(options_json, base::JSON_PARSE_RFC);
  if (!parsed) {
    return base::unexpected("options must be a JSON object");
  }

  CacheOptions options;
  for (const auto [key, value] : *parsed) {
    if (key == "cacheDir") {
      if (!value.is_string()) {
        return base::unexpected("cacheDir must be a string");
      }
      options.directory = base::FilePath::FromUTF8Unsafe(value.GetString());
    } else if (key == "urls") {
      if (!value.is_list()) {
        return base::unexpected("urls must be an array");
      }
      for (const base::Value& url : value.GetList()) {
        if (!url.is_string()) {
          return base::unexpected("urls must be an array of strings");
        }
        options.clear.urls.push_back(url.GetString());
      }
    } else if (key == "unusedSinceMs") {
      if (!value.is_double() && !value.is_int()) {
        return base::unexpected("unusedSinceMs must be a number");
      }
      options.clear.unused_since =
          base::Time::FromMillisecondsSinceUnixEpoch(value.GetDouble());
    } else if (key == "maxBytes") {
      if (!value.is_double() && !value.is_int()) {
        return base::unexpected("maxBytes must be a number");
      }
      if (value.GetDouble() < 0) {
        return base::unexpected("maxBytes cannot be negative");
      }
      options.clear.max_bytes = static_cast<int64_t>(value.GetDouble());
    } else {
      return base::unexpected("unknown option \"" + key + "\"");
    }
  }
  if (options.directory.empty()) {
    return base::unexpected("cacheDir is required");
  }
  return options;
}

std::string EntriesToJson(const std::vector<CacheEntry>& entries) {
  base::ListValue list;
  for (const CacheEntry& entry : entries) {
    base::DictValue item;
    item.Set("url", entry.url);
    item.Set("lastUsedMs", entry.last_used.InMillisecondsFSinceUnixEpoch());
    item.Set("bytes", static_cast<double>(entry.size));
    list.Append(std::move(item));
  }
  std::string json;
  base::JSONWriter::Write(list, &json);
  return json;
}

std::string ClearResultToJson(const CacheClearResult& result) {
  base::DictValue dict;
  dict.Set("removed", result.removed);
  dict.Set("bytesBefore", static_cast<double>(result.bytes_before));
  dict.Set("bytesAfter", static_cast<double>(result.bytes_after));
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

// Runs `work` against a backend for `directory`, wherever one can be had.
//
// The three cases, in the order they are checked: the engine already has this
// exact directory open, so borrow its backend; there is an engine but it has
// some other directory, so open this one on the engine's thread, which has the
// environment; there is no engine, so build an environment and open it there.
//
// The first case is not an optimisation. Within a process, disk_cache
// sequences backends per directory, so opening a second one for a directory
// the engine holds does not fail -- it waits for the engine's to go away,
// which it will not do while the engine is up. Borrowing is the only thing
// that returns.
//
// Takes the thread rather than the shot_engine because shot_engine is defined
// below this namespace, where a C header's opaque type has to be completed.
void WithCacheBackend(
    EngineThread* engine,
    const base::FilePath& directory,
    base::OnceCallback<void(disk_cache::Backend*, const std::string&)> work) {
  // No engine now, but there was one: its ShotRuntime shut the process-wide
  // ThreadPoolInstance down on the way out, and a thread pool cannot be
  // restarted -- Create() refuses while an instance exists, and the existing
  // one drops everything posted to it. The disk cache does its file I/O
  // through that pool, so opening a backend here would post work that never
  // runs and then wait for a callback that never comes.
  //
  // Reported rather than hung, and reported with the two things that do work:
  // the same call while the engine is up borrows its backend, and clearing a
  // whole directory needs no backend at all -- the JS layer removes the
  // directory outright for that case, which is safe precisely because nothing
  // survives to disagree with the index.
  if (!engine && EngineWasCreated()) {
    std::move(work).Run(
        nullptr,
        "this process has already run an engine, and shutting it down shut "
        "down the thread pool the cache backend needs. Inspect the cache "
        "while the engine is up, or before starting it; clearing a whole "
        "directory works either way");
    return;
  }

  auto run = base::BindOnce(
      [](const base::FilePath& directory,
         base::OnceCallback<void(disk_cache::Backend*, const std::string&)>
             work,
         bool may_borrow) {
        if (may_borrow && ShotNetwork::CacheDir() == directory) {
          disk_cache::Backend* backend = ShotNetwork::CacheBackend();
          if (backend) {
            std::move(work).Run(backend, std::string());
            return;
          }
          // The engine has this directory configured but has not built its
          // backend yet, which happens when nothing has been fetched. Opening
          // it here would race the engine doing the same, so report the state
          // rather than the lock error it would turn into.
          std::move(work).Run(
              nullptr,
              "the engine has this cache directory but has not opened it yet; "
              "take a screenshot first, or ask about it before starting the "
              "engine");
          return;
        }
        auto backend = OpenCacheBackend(directory);
        if (!backend.has_value()) {
          std::move(work).Run(nullptr, backend.error());
          return;
        }
        std::move(work).Run(backend->get(), std::string());
      },
      directory, std::move(work), engine != nullptr);

  if (engine) {
    engine->RunSync(std::move(run));
    return;
  }
  CacheThread thread(std::move(run));
  thread.RunAndJoin();
}

}  // namespace
}  // namespace shot

struct shot_engine {
  shot::EngineThread thread;
};

extern "C" {

int32_t shot_abi_version(void) {
  return SHOT_ABI_VERSION;
}

const uint8_t* shot_buffer_data(const shot_buffer* buffer) {
  return buffer ? buffer->bytes.data() : nullptr;
}

size_t shot_buffer_size(const shot_buffer* buffer) {
  return buffer ? buffer->size : 0;
}

void shot_buffer_free(shot_buffer* buffer) {
  delete buffer;
}

shot_status shot_engine_create(const char* options_json,
                               shot_engine** out_engine,
                               shot_buffer** out_error) {
  shot::Clear(out_engine);
  shot::Clear(out_error);
  if (!out_engine) {
    shot::Deliver(out_error, "shot_engine_create needs somewhere to put the "
                             "engine");
    return SHOT_ERR_USAGE;
  }

  auto options = shot::ParseEngineOptions(options_json);
  if (!options.has_value()) {
    shot::Deliver(out_error, options.error());
    return SHOT_ERR_USAGE;
  }

  if (shot::EngineWasCreated().exchange(true)) {
    shot::Deliver(out_error,
                  "this process has already had an engine; blink can only be "
                  "started once, and destroying one does not give the process "
                  "back the ability to make another");
    return SHOT_ERR_STATE;
  }

  auto engine = std::make_unique<shot_engine>();
  std::string error;
  if (!engine->thread.Start(std::move(options).value(), &error)) {
    shot::Deliver(out_error, error);
    return SHOT_ERR_STATE;
  }

  *out_engine = engine.release();
  return SHOT_OK;
}

void shot_engine_destroy(shot_engine* engine) {
  if (!engine) {
    return;
  }
  engine->thread.Stop();
  delete engine;
}

shot_status shot_engine_capture(shot_engine* engine,
                                const char* request_json,
                                shot_buffer** out_image,
                                shot_buffer** out_stats,
                                shot_buffer** out_error) {
  shot::Clear(out_image);
  shot::Clear(out_stats);
  shot::Clear(out_error);
  if (!engine || !out_image) {
    shot::Deliver(out_error,
                  "shot_engine_capture needs an engine and somewhere to put "
                  "the image");
    return SHOT_ERR_USAGE;
  }
  if (!request_json) {
    shot::Deliver(out_error, "shot_engine_capture needs a request");
    return SHOT_ERR_USAGE;
  }

  // Filled in on the engine thread; read here only after RunSync returns,
  // which is what makes the plain locals safe.
  shot_status status = SHOT_ERR_CAPTURE;
  shot::Bytes image;
  std::string error;
  std::string stats;

  engine->thread.RunSync(base::BindOnce(
      [](shot_engine* engine, const char* request_json, shot_status* status,
         shot::Bytes* image, std::string* stats, std::string* error) {
        auto request = shot::ParseScreenshotRequest(
            request_json, engine->thread.default_allow_file_access());
        if (!request.has_value()) {
          // No statistics for a request that was never run. The alternative --
          // an object of zeroes -- would read as "nothing was fetched", which
          // is true and misleading in the same breath.
          *status = SHOT_ERR_USAGE;
          *error = request.error();
          return;
        }
        shot::CaptureStats collected;
        auto result = shot::CaptureAndDeliver(engine->thread.runtime(),
                                              *request, &collected);
        // Serialised before the failure is checked: a capture that got part of
        // the way through has counters worth returning, and they are the same
        // counters either way.
        *stats = shot::StatsToJson(collected);
        if (!result.has_value()) {
          *status = SHOT_ERR_CAPTURE;
          *error = result.error();
          return;
        }
        *image = std::move(result->image);
        *status = SHOT_OK;
      },
      base::Unretained(engine), request_json, base::Unretained(&status),
      base::Unretained(&image), base::Unretained(&stats),
      base::Unretained(&error)));

  // Before the status is acted on, because a failed capture reports statistics
  // too -- see the header. An empty string means the request never got as far
  // as running, and nothing is delivered for it.
  if (out_stats && !stats.empty()) {
    *out_stats = shot::MakeMessage(stats);
  }
  if (status != SHOT_OK) {
    shot::Deliver(out_error, error);
    return status;
  }
  *out_image = shot::MakeImage(std::move(image));
  return SHOT_OK;
}

struct shot_tile_list {
  struct Entry {
    gfx::Rect region;
    std::string path;
    // Null once taken by shot_tile_list_take_image(), or when the tile went
    // to a path and there were no bytes to hand back.
    shot_buffer* image = nullptr;
  };
  std::vector<Entry> tiles;
};

shot_status shot_engine_capture_tiles(shot_engine* engine,
                                      const char* request_json,
                                      shot_tile_list** out_tiles,
                                      shot_buffer** out_stats,
                                      shot_buffer** out_error) {
  shot::Clear(out_tiles);
  shot::Clear(out_stats);
  shot::Clear(out_error);
  if (!engine || !out_tiles) {
    shot::Deliver(out_error,
                  "shot_engine_capture_tiles needs an engine and somewhere to "
                  "put the tiles");
    return SHOT_ERR_USAGE;
  }
  if (!request_json) {
    shot::Deliver(out_error, "shot_engine_capture_tiles needs a request");
    return SHOT_ERR_USAGE;
  }

  shot_status status = SHOT_ERR_CAPTURE;
  std::vector<shot::DeliveredTile> tiles;
  std::string error;
  std::string stats;

  engine->thread.RunSync(base::BindOnce(
      [](shot_engine* engine, const char* request_json, shot_status* status,
         std::vector<shot::DeliveredTile>* tiles, std::string* stats,
         std::string* error) {
        auto request = shot::ParseScreenshotRequest(
            request_json, engine->thread.default_allow_file_access());
        if (!request.has_value()) {
          *status = SHOT_ERR_USAGE;
          *error = request.error();
          return;
        }
        if (!request->tile.has_value()) {
          *status = SHOT_ERR_USAGE;
          *error = "shot_engine_capture_tiles needs tile.height in the request";
          return;
        }
        shot::CaptureStats collected;
        auto result = shot::CaptureTiles(engine->thread.runtime(), *request,
                                         &collected);
        *stats = shot::StatsToJson(collected);
        if (!result.has_value()) {
          *status = SHOT_ERR_CAPTURE;
          *error = result.error();
          return;
        }
        *tiles = std::move(*result);
        *status = SHOT_OK;
      },
      base::Unretained(engine), request_json, base::Unretained(&status),
      base::Unretained(&tiles), base::Unretained(&stats),
      base::Unretained(&error)));

  if (out_stats && !stats.empty()) {
    *out_stats = shot::MakeMessage(stats);
  }
  if (status != SHOT_OK) {
    shot::Deliver(out_error, error);
    return status;
  }
  auto* list = new shot_tile_list;
  list->tiles.reserve(tiles.size());
  for (shot::DeliveredTile& tile : tiles) {
    shot_tile_list::Entry entry;
    entry.region = tile.region;
    entry.path = std::move(tile.path);
    entry.image = shot::MakeImage(std::move(tile.image));
    list->tiles.push_back(std::move(entry));
  }
  *out_tiles = list;
  return SHOT_OK;
}

size_t shot_tile_list_count(const shot_tile_list* tiles) {
  return tiles ? tiles->tiles.size() : 0;
}

void shot_tile_list_region(const shot_tile_list* tiles,
                           size_t index,
                           int32_t* out_x,
                           int32_t* out_y,
                           int32_t* out_width,
                           int32_t* out_height) {
  shot::Clear(out_x);
  shot::Clear(out_y);
  shot::Clear(out_width);
  shot::Clear(out_height);
  if (!tiles || index >= tiles->tiles.size()) {
    return;
  }
  const gfx::Rect& region = tiles->tiles[index].region;
  if (out_x) {
    *out_x = region.x();
  }
  if (out_y) {
    *out_y = region.y();
  }
  if (out_width) {
    *out_width = region.width();
  }
  if (out_height) {
    *out_height = region.height();
  }
}

const char* shot_tile_list_path(const shot_tile_list* tiles, size_t index) {
  if (!tiles || index >= tiles->tiles.size() ||
      tiles->tiles[index].path.empty()) {
    return nullptr;
  }
  return tiles->tiles[index].path.c_str();
}

shot_buffer* shot_tile_list_take_image(shot_tile_list* tiles, size_t index) {
  if (!tiles || index >= tiles->tiles.size()) {
    return nullptr;
  }
  return std::exchange(tiles->tiles[index].image, nullptr);
}

void shot_tile_list_free(shot_tile_list* tiles) {
  if (!tiles) {
    return;
  }
  for (shot_tile_list::Entry& entry : tiles->tiles) {
    shot_buffer_free(entry.image);
  }
  delete tiles;
}

shot_status shot_engine_status(shot_engine* engine,
                               shot_buffer** out_json,
                               shot_buffer** out_error) {
  shot::Clear(out_json);
  shot::Clear(out_error);
  if (!engine || !out_json) {
    shot::Deliver(out_error,
                  "shot_engine_status needs an engine and somewhere to put "
                  "the answer");
    return SHOT_ERR_USAGE;
  }

  std::string json;
  // On the engine thread because that is where ShotNetwork's statics were
  // written; reading them from here would be a data race that happens to work
  // on every machine anyone has tried it on.
  engine->thread.RunSync(base::BindOnce(
      [](std::string* json) {
        const base::FilePath& dir = shot::ShotNetwork::CacheDir();
        base::DictValue dict;
        if (dir.empty()) {
          dict.Set("cacheDir", base::Value());
        } else {
          dict.Set("cacheDir", dir.AsUTF8Unsafe());
        }
        dict.Set("cacheActive", shot::ShotNetwork::CacheActive());
        base::JSONWriter::Write(dict, json);
      },
      base::Unretained(&json)));

  *out_json = shot::MakeMessage(json);
  return SHOT_OK;
}

shot_status shot_cache_list(shot_engine* engine,
                            const char* options_json,
                            shot_buffer** out_json,
                            shot_buffer** out_error) {
  shot::Clear(out_json);
  shot::Clear(out_error);
  if (!out_json) {
    shot::Deliver(out_error, "shot_cache_list needs somewhere to put the list");
    return SHOT_ERR_USAGE;
  }
  auto options = shot::ParseCacheOptions(options_json);
  if (!options.has_value()) {
    shot::Deliver(out_error, options.error());
    return SHOT_ERR_USAGE;
  }

  std::string json;
  std::string error;
  shot::WithCacheBackend(
      engine ? &engine->thread : nullptr, options->directory,
      base::BindOnce(
          [](std::string* json, std::string* error,
             disk_cache::Backend* backend, const std::string& failure) {
            if (!backend) {
              *error = failure;
              return;
            }
            auto entries = shot::ListCacheEntries(backend);
            if (!entries.has_value()) {
              *error = entries.error();
              return;
            }
            *json = shot::EntriesToJson(*entries);
          },
          base::Unretained(&json), base::Unretained(&error)));

  if (!error.empty()) {
    shot::Deliver(out_error, error);
    return SHOT_ERR_STATE;
  }
  *out_json = shot::MakeMessage(json);
  return SHOT_OK;
}

shot_status shot_cache_clear(shot_engine* engine,
                             const char* options_json,
                             shot_buffer** out_json,
                             shot_buffer** out_error) {
  shot::Clear(out_json);
  shot::Clear(out_error);
  if (!out_json) {
    shot::Deliver(out_error,
                  "shot_cache_clear needs somewhere to put the result");
    return SHOT_ERR_USAGE;
  }
  auto options = shot::ParseCacheOptions(options_json);
  if (!options.has_value()) {
    shot::Deliver(out_error, options.error());
    return SHOT_ERR_USAGE;
  }

  std::string json;
  std::string error;
  shot::WithCacheBackend(
      engine ? &engine->thread : nullptr, options->directory,
      base::BindOnce(
          [](const shot::CacheClearOptions& clear, std::string* json,
             std::string* error, disk_cache::Backend* backend,
             const std::string& failure) {
            if (!backend) {
              *error = failure;
              return;
            }
            auto result = shot::ClearCacheEntries(backend, clear);
            if (!result.has_value()) {
              *error = result.error();
              return;
            }
            *json = shot::ClearResultToJson(*result);
          },
          options->clear, base::Unretained(&json), base::Unretained(&error)));

  if (!error.empty()) {
    shot::Deliver(out_error, error);
    return SHOT_ERR_STATE;
  }
  *out_json = shot::MakeMessage(json);
  return SHOT_OK;
}

void shot_engine_purge(shot_engine* engine, int32_t release_working_set) {
  if (!engine) {
    return;
  }
  engine->thread.RunSync(base::BindOnce(
      [](shot::ShotRuntime* runtime, bool release) {
        runtime->PurgeMemory();
        if (release) {
          runtime->ReleaseWorkingSet();
        }
      },
      base::Unretained(&engine->thread.runtime()),
      release_working_set != 0));
}

}  // extern "C"
