// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_engine.h"

#include <atomic>
#include <cstdlib>
#include <memory>
#include <mutex>
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
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/no_destructor.h"
#include "base/path_service.h"
#include "base/run_loop.h"
#include "base/synchronization/lock.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/single_thread_task_executor.h"
#include "base/task/single_thread_task_runner.h"
#include "base/task/thread_pool/thread_pool_instance.h"
#include "base/threading/simple_thread.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/disk_cache/backend_cleanup_tracker.h"
#include "net/disk_cache/disk_cache.h"
#include "shot/shot_bytes.h"
#include "shot/shot_cache.h"
#include "shot/shot_capture.h"
#include "shot/shot_capture_context.h"
#include "shot/shot_network.h"
#include "shot/shot_request.h"
#include "shot/shot_runtime.h"

namespace shot {
base::expected<EngineOptions, std::string> ReadEngineOptions(
    const base::DictValue& dict) {
  EngineOptions options;
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

base::expected<CacheOptions, std::string> ReadCacheOptions(
    const base::DictValue& dict) {
  CacheOptions options;
  for (const auto [key, value] : dict) {
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

namespace {
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
    accepting_ = true;
    return true;
  }

  void Stop() {
    std::lock_guard stop_lock(stop_mutex_);
    {
      std::lock_guard lock(mutex_);
      if (!thread_) {
        return;
      }
      accepting_ = false;
      CHECK(task_runner_->PostNonNestableTask(FROM_HERE, quit_));
    }
    thread_->Join();
    thread_.reset();
  }

  bool Post(base::OnceClosure task) {
    std::lock_guard lock(mutex_);
    if (!accepting_) {
      return false;
    }
    return task_runner_->PostNonNestableTask(FROM_HERE, std::move(task));
  }

  bool RunSync(base::OnceClosure task) {
    base::WaitableEvent done;
    if (!Post(base::BindOnce(
            [](base::OnceClosure task, base::WaitableEvent* done) {
              std::move(task).Run();
              done->Signal();
            },
            std::move(task), base::Unretained(&done)))) {
      return false;
    }
    done.Wait();
    return true;
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
    if (!base::CommandLine::InitializedForCurrentProcess()) {
      base::CommandLine::Init(0, nullptr);
    }

    // stderr, matching the executable. A library writing to the host's stdout
    // would be worse than noise -- for shotium's addon that stream belongs to
    // whatever the host program is saying.
    logging::LoggingSettings log_settings;
    log_settings.logging_dest = logging::LOG_TO_STDERR;
    logging::InitLogging(log_settings);
    // Warnings only, as a library in someone else's process should be.
    // SHOT_VERBOSE=1 is the executable's --verbose for a host that cannot pass
    // one, which is how the SHOT_PROFILE lines are read through the addon.
    const char* verbose = std::getenv("SHOT_VERBOSE");
    logging::SetMinLogLevel(verbose && *verbose && *verbose != '0'
                                ? logging::LOGGING_INFO
                                : logging::LOGGING_WARNING);

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

  std::mutex mutex_;
  std::mutex stop_mutex_;
  bool accepting_ = false;
  EngineOptions options_;
  std::unique_ptr<base::DelegateSimpleThread> thread_;
  base::WaitableEvent ready_;
  // Written on the engine thread before ready_, read after it.
  std::string error_;
  raw_ptr<ShotRuntime> runtime_ = nullptr;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::RepeatingClosure quit_;
};

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
      // Shutdown only drains shutdown-blocking work; it does not join workers.
      // This isolated embedding environment owns every task and must join
      // before destroying the pool. Chromium exposes that join primitive as
      // JoinForTesting because its browser normally leaks the pool until exit.
      base::ThreadPoolInstance::Get()->JoinForTesting();
      base::ThreadPoolInstance::Set(nullptr);
    }
  }

  base::OnceClosure task_;
};

base::Value EntriesToValue(const std::vector<CacheEntry>& entries) {
  base::ListValue list;
  for (const CacheEntry& entry : entries) {
    base::DictValue item;
    item.Set("url", entry.url);
    item.Set("lastUsedMs", entry.last_used.InMillisecondsFSinceUnixEpoch());
    item.Set("bytes", static_cast<double>(entry.size));
    list.Append(std::move(item));
  }
  return base::Value(std::move(list));
}

base::Value ClearResultToValue(const CacheClearResult& result) {
  base::DictValue dict;
  dict.Set("removed", result.removed);
  dict.Set("bytesBefore", static_cast<double>(result.bytes_before));
  dict.Set("bytesAfter", static_cast<double>(result.bytes_after));
  return base::Value(std::move(dict));
}

// Serializes temporary cache environments with engine initialization.
base::Lock& InitializationMutex() {
  static base::NoDestructor<base::Lock> mutex;
  return *mutex;
}

EngineResult RunCache(const CacheOptions& options, bool clearing, bool borrow) {
  EngineResult result;
  auto run = [&](disk_cache::Backend* backend) {
    if (clearing) {
      auto answer = ClearCacheEntries(backend, options.clear);
      if (answer.has_value()) {
        result.value = ClearResultToValue(*answer);
      } else {
        result.error = answer.error();
      }
    } else {
      auto answer = ListCacheEntries(backend);
      if (answer.has_value()) {
        result.value = EntriesToValue(*answer);
      } else {
        result.error = answer.error();
      }
    }
  };
  if (borrow && ShotNetwork::CacheDir() == options.directory) {
    if (auto* backend = ShotNetwork::CacheBackend()) {
      run(backend);
    } else {
      result.error =
          "the engine has this cache directory but has not opened it yet; take "
          "a screenshot first, or ask about it before starting the engine";
    }
  } else {
    auto backend = OpenCacheBackend(options.directory);
    if (backend.has_value()) {
      run(backend->get());
      backend->reset();
      // The backend destructor schedules index cleanup back onto this IO
      // sequence. Keep pumping until the directory is reusable, before a
      // temporary cache environment tears down its executor and thread pool.
      base::RunLoop cleanup(base::RunLoop::Type::kNestableTasksAllowed);
      auto released = disk_cache::BackendCleanupTracker::TryCreate(
          options.directory, cleanup.QuitClosure());
      if (!released) {
        cleanup.Run();
      }
    } else {
      result.error = backend.error();
    }
  }
  if (!result.error.empty()) {
    result.status = EngineStatus::kState;
  }
  return result;
}

EngineResult Closed() {
  EngineResult result;
  result.status = EngineStatus::kState;
  result.error = "shotium: this engine has been destroyed";
  return result;
}
}  // namespace

class EngineService::Impl {
 public:
  EngineThread thread;
};
EngineService::EngineService() : impl_(std::make_unique<Impl>()) {}
EngineService::~EngineService() {
  Stop();
}
base::expected<std::shared_ptr<EngineService>, std::string>
EngineService::Create(EngineOptions options) {
  base::AutoLock lock(InitializationMutex());
  if (EngineWasCreated().exchange(true)) {
    return base::unexpected(
        "this process has already had an engine; blink can only be started "
        "once, and destroying one does not give the process back the ability "
        "to make another");
  }
  auto service = std::shared_ptr<EngineService>(new EngineService);
  std::string error;
  if (!service->impl_->thread.Start(std::move(options), &error)) {
    return base::unexpected(error);
  }
  return service;
}
void EngineService::Stop() {
  impl_->thread.Stop();
}
bool EngineService::default_allow_file_access() const {
  return impl_->thread.default_allow_file_access();
}

bool EngineService::Capture(ScreenshotRequest request,
                            bool tiles,
                            EngineCompletion completion) {
  return impl_->thread.Post(base::BindOnce(
      [](Impl* impl, ScreenshotRequest request, bool tiles,
         EngineCompletion done) {
        EngineResult result;
        if (tiles && !request.tile) {
          result.status = EngineStatus::kUsage;
          result.error =
              "shot_engine_capture_tiles needs tile.height in the request";
        } else {
          result.stats.emplace();
          if (tiles) {
            auto captured =
                CaptureTiles(impl->thread.runtime(), request, &*result.stats);
            if (captured.has_value()) {
              result.tiles = std::move(*captured);
            } else {
              result.error = captured.error();
            }
          } else {
            auto captured = CaptureAndDeliver(impl->thread.runtime(), request,
                                              &*result.stats);
            if (captured.has_value()) {
              result.image = std::move(captured->image);
            } else {
              result.error = captured.error();
            }
          }
          if (!result.error.empty()) {
            result.status = EngineStatus::kCapture;
          }
        }
        std::move(done).Run(std::move(result));
      },
      base::Unretained(impl_.get()), std::move(request), tiles,
      std::move(completion)));
}

bool EngineService::Cache(CacheOptions options,
                          bool clearing,
                          EngineCompletion completion) {
  return impl_->thread.Post(base::BindOnce(
      [](CacheOptions options, bool clearing, EngineCompletion done) {
        std::move(done).Run(RunCache(options, clearing, true));
      },
      std::move(options), clearing, std::move(completion)));
}
EngineResult EngineService::CaptureSync(ScreenshotRequest request, bool tiles) {
  EngineResult result;
  base::WaitableEvent done;
  if (!Capture(std::move(request), tiles,
               base::BindOnce(
                   [](EngineResult* out, base::WaitableEvent* done,
                      EngineResult value) {
                     *out = std::move(value);
                     done->Signal();
                   },
                   base::Unretained(&result), base::Unretained(&done)))) {
    return Closed();
  }
  done.Wait();
  return result;
}
EngineResult EngineService::CacheSync(CacheOptions options, bool clearing) {
  EngineResult result;
  base::WaitableEvent done;
  if (!Cache(std::move(options), clearing,
             base::BindOnce(
                 [](EngineResult* out, base::WaitableEvent* done,
                    EngineResult value) {
                   *out = std::move(value);
                   done->Signal();
                 },
                 base::Unretained(&result), base::Unretained(&done)))) {
    return Closed();
  }
  done.Wait();
  return result;
}
base::DictValue EngineService::Status() {
  base::DictValue result;
  impl_->thread.RunSync(base::BindOnce(
      [](base::DictValue* result) {
        const auto& dir = ShotNetwork::CacheDir();
        result->Set("cacheDir", dir.empty() ? base::Value()
                                            : base::Value(dir.AsUTF8Unsafe()));
        result->Set("cacheActive", ShotNetwork::CacheActive());
      },
      base::Unretained(&result)));
  return result;
}
void EngineService::Purge(bool release) {
  impl_->thread.RunSync(base::BindOnce(
      [](Impl* impl, bool release) {
        impl->thread.runtime().PurgeMemory();
        if (release) {
          impl->thread.runtime().ReleaseWorkingSet();
        }
      },
      base::Unretained(impl_.get()), release));
}
EngineResult CacheWithoutEngine(CacheOptions options, bool clearing) {
  base::AutoLock lock(InitializationMutex());
  if (EngineWasCreated()) {
    auto result = Closed();
    result.error =
        "this process has already run an engine, and shutting it down shut "
        "down the thread pool the cache backend needs. Inspect the cache while "
        "the engine is up, or before starting it; clearing a whole directory "
        "works either way";
    return result;
  }
  EngineResult result;
  CacheThread thread(base::BindOnce(
      [](CacheOptions options, bool clearing, EngineResult* result) {
        *result = RunCache(options, clearing, false);
      },
      std::move(options), clearing, base::Unretained(&result)));
  thread.RunAndJoin();
  return result;
}
}  // namespace shot
