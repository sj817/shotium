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
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/simple_thread.h"
#include "base/values.h"
#include "shot/shot_capture.h"
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
  std::vector<uint8_t> bytes;
  size_t size = 0;
};

namespace shot {
namespace {

shot_buffer* MakeImage(std::vector<uint8_t> bytes) {
  auto* buffer = new shot_buffer;
  buffer->size = bytes.size();
  buffer->bytes = std::move(bytes);
  return buffer;
}

shot_buffer* MakeMessage(std::string_view text) {
  auto* buffer = new shot_buffer;
  buffer->bytes.assign(text.begin(), text.end());
  buffer->size = buffer->bytes.size();
  buffer->bytes.push_back(0);
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

    // Where shot_data.pak and shot_strings.pak are. See the header: the
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
                                shot_buffer** out_error) {
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
  std::vector<uint8_t> image;
  std::string error;

  engine->thread.RunSync(base::BindOnce(
      [](shot_engine* engine, const char* request_json, shot_status* status,
         std::vector<uint8_t>* image, std::string* error) {
        auto request = shot::ParseScreenshotRequest(
            request_json, engine->thread.default_allow_file_access());
        if (!request.has_value()) {
          *status = SHOT_ERR_USAGE;
          *error = request.error();
          return;
        }
        auto result = shot::CaptureAndDeliver(*request);
        if (!result.has_value()) {
          *status = SHOT_ERR_CAPTURE;
          *error = result.error();
          return;
        }
        *image = std::move(result->image);
        *status = SHOT_OK;
      },
      base::Unretained(engine), request_json, base::Unretained(&status),
      base::Unretained(&image), base::Unretained(&error)));

  if (status != SHOT_OK) {
    shot::Deliver(out_error, error);
    return status;
  }
  *out_image = shot::MakeImage(std::move(image));
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
