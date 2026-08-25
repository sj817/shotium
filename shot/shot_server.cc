// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_server.h"

#include <cstdint>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/memory/raw_ref.h"
#include "base/numerics/byte_conversions.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_runner.h"
#include "base/threading/simple_thread.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "base/values.h"
#include "build/build_config.h"
#include "shot/shot_capture.h"
#include "shot/shot_options.h"
#include "shot/shot_request.h"
#include "shot/shot_runtime.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace shot {
namespace {

// A request is JSON describing one screenshot; nothing legitimate approaches
// this. The cap exists so that a desynchronised stream -- which reads as an
// enormous length -- fails immediately instead of trying to allocate whatever
// four arbitrary bytes happened to say.
constexpr uint32_t kMaximumRequestBytes = 8u * 1024u * 1024u;

// How long the request stream has to stay quiet before a worker decides its
// batch is over and hands back everything it was keeping warm.
//
// The number is a bet about callers, and it is a cheap bet either way. Too
// short and a caller taking screenshots in a loop pays for a purge it is about
// to undo -- but only if it pauses for two seconds mid-loop, and a purge costs
// a few milliseconds. Too long and a resident daemon sits on a warm cache
// nobody is going to read. Two seconds is well past any gap inside a batch and
// well inside the gap between batches.
constexpr base::TimeDelta kIdleBeforePurge = base::Seconds(2);

// And how long before it gives the pages back too.
//
// A separate and longer silence, because the price is different. A purge
// throws away caches that were going cold anyway; a trim is paid back in soft
// faults on the very next request, measured at about 8 ms for a settled
// worker. Ten seconds is the line between "the caller is between batches" and
// "nobody is asking", and 8 ms once on the far side of it buys back about
// 25 MB per worker -- almost all of it shot.exe's own code, faulted in during
// startup by paths that will not run again.
constexpr base::TimeDelta kIdleBeforeTrim = base::Seconds(10);

base::File StandardInput() {
#if BUILDFLAG(IS_WIN)
  return base::File(::GetStdHandle(STD_INPUT_HANDLE));
#else
  return base::File(STDIN_FILENO);
#endif
}

base::File StandardOutput() {
#if BUILDFLAG(IS_WIN)
  return base::File(::GetStdHandle(STD_OUTPUT_HANDLE));
#else
  return base::File(STDOUT_FILENO);
#endif
}

// Reads exactly `data.size()` bytes, or reports why not. A pipe is free to hand
// back a short read at any point, so this loops; `false` with `at_eof` set is
// the supervisor having closed the pipe, which is a clean exit rather than a
// failure.
bool ReadExactly(base::File& input, base::span<uint8_t> data, bool* at_eof) {
  *at_eof = false;
  size_t total = 0;
  while (total < data.size()) {
    std::optional<size_t> read = input.ReadAtCurrentPos(data.subspan(total));
    if (!read.has_value() || *read == 0) {
      // The stream ended. Clean only if it lands on a frame boundary: there is
      // no partial request to answer and nothing to recover, so the supervisor
      // closing the pipe is how a worker is told to stop. Halfway through a
      // frame it is a real failure, and saying so beats pretending the stream
      // ended tidily.
      //
      // Both branches, because the two platforms report the same event
      // differently: POSIX gives a zero-length read, while Windows fails the
      // read outright with ERROR_BROKEN_PIPE once the writer is gone.
      *at_eof = (total == 0);
      return false;
    }
    total += *read;
  }
  return true;
}

bool WriteFrame(base::File& output, base::span<const uint8_t> payload) {
  const std::array<uint8_t, 4> header =
      base::U32ToLittleEndian(static_cast<uint32_t>(payload.size()));
  if (!output.WriteAtCurrentPosAndCheck(header)) {
    return false;
  }
  return payload.empty() || output.WriteAtCurrentPosAndCheck(payload);
}

bool WriteResponse(base::File& output,
                   base::DictValue header,
                   base::span<const uint8_t> payload) {
  std::optional<std::string> json = base::WriteJson(header);
  if (!json) {
    // Nothing sensible left to say on the wire; the header is built here, so
    // this would be a bug in this file rather than in the request.
    LOG(ERROR) << "shot: could not serialise the response header";
    return false;
  }
  return WriteFrame(output, base::as_byte_span(*json)) &&
         WriteFrame(output, payload);
}

base::DictValue ErrorHeader(const std::string& message) {
  base::DictValue header;
  header.Set("ok", false);
  header.Set("error", message);
  return header;
}

// The request stream, read on a thread that is allowed to block on it.
//
// It hands whole frames to the rendering thread and never looks at what is in
// them: parsing is the renderer's, so that a malformed request is answered by
// the same code that answers a good one. Reading ahead is harmless -- tasks
// run in the order they were posted, so answers go back in the order the
// requests arrived -- and in practice does not happen, because the supervisor
// keeps one request in flight per worker.
class RequestReader : public base::DelegateSimpleThread::Delegate {
 public:
  RequestReader(base::File input,
                scoped_refptr<base::SingleThreadTaskRunner> renderer,
                base::RepeatingCallback<void(std::vector<uint8_t>)> on_request,
                base::OnceCallback<void(int)> on_end)
      : input_(std::move(input)),
        renderer_(std::move(renderer)),
        on_request_(std::move(on_request)),
        on_end_(std::move(on_end)) {}

  // base::DelegateSimpleThread::Delegate:
  void Run() override {
    const int code = ReadUntilTheStreamEnds();
    renderer_->PostTask(FROM_HERE, base::BindOnce(std::move(on_end_), code));
  }

 private:
  int ReadUntilTheStreamEnds() {
    for (;;) {
      std::array<uint8_t, 4> length_bytes = {};
      bool at_eof = false;
      if (!ReadExactly(input_, length_bytes, &at_eof)) {
        if (at_eof) {
          return kSuccessExitCode;
        }
        LOG(ERROR) << "shot: the request stream ended inside a frame";
        return kCaptureExitCode;
      }

      const uint32_t length = base::U32FromLittleEndian(length_bytes);
      if (length == 0 || length > kMaximumRequestBytes) {
        // Not recoverable: the stream is out of step, and every later frame
        // boundary would be wrong too.
        LOG(ERROR) << "shot: request frame of " << length
                   << " bytes is out of range; the stream is desynchronised";
        return kCaptureExitCode;
      }

      std::vector<uint8_t> request_bytes(length, 0);
      if (!ReadExactly(input_, request_bytes, &at_eof)) {
        LOG(ERROR) << "shot: the request stream ended inside a frame";
        return kCaptureExitCode;
      }
      renderer_->PostTask(FROM_HERE,
                          base::BindOnce(on_request_, std::move(request_bytes)));
    }
  }

  base::File input_;
  const scoped_refptr<base::SingleThreadTaskRunner> renderer_;
  const base::RepeatingCallback<void(std::vector<uint8_t>)> on_request_;
  base::OnceCallback<void(int)> on_end_;
};

// The rendering half: one request at a time, on the thread blink lives on.
class RequestHandler {
 public:
  RequestHandler(ShotRuntime& runtime,
                 base::File output,
                 bool default_allow_file_access,
                 base::OnceClosure quit)
      : runtime_(runtime),
        output_(std::move(output)),
        default_allow_file_access_(default_allow_file_access),
        quit_(std::move(quit)) {
    // Armed before the first request rather than after it, so that a worker
    // the pool started and has not used yet gives back what starting cost it.
    ArmIdleTimer();
  }


  int exit_code() const { return exit_code_; }

  void OnRequest(std::vector<uint8_t> bytes) {
    idle_timer_.Stop();
    purged_ = false;
    if (!Answer(bytes)) {
      // stdout is gone, so there is no way to report anything and no reason to
      // read another request. In practice stdin is gone too and the reader is
      // about to say so; this is the case where it is not.
      Finish(kCaptureExitCode);
      return;
    }
    ArmIdleTimer();
  }

  void OnStreamEnd(int code) { Finish(code); }

 private:
  void ArmIdleTimer() {
    idle_timer_.Start(FROM_HERE,
                      purged_ ? kIdleBeforeTrim : kIdleBeforePurge,
                      base::BindOnce(&RequestHandler::OnIdle,
                                     base::Unretained(this)));
  }

  // The silence has gone on long enough for the next thing. Purging comes
  // first and starts the clock on the trim; the trim does not re-arm, because
  // there is nothing left after it that another silence would improve.
  void OnIdle() {
    if (!purged_) {
      runtime_->PurgeMemory();
      purged_ = true;
      ArmIdleTimer();
      return;
    }
    runtime_->ReleaseWorkingSet();
  }

  void Finish(int code) {
    idle_timer_.Stop();
    exit_code_ = code;
    if (quit_) {
      std::move(quit_).Run();
    }
  }

  // One request, from bytes on the wire to bytes back on it. `false` means the
  // response could not be written, which is the only failure that ends the
  // worker -- a request that cannot be parsed or rendered is answered.
  bool Answer(base::span<const uint8_t> bytes) {
    auto request = ParseScreenshotRequest(
        std::string_view(reinterpret_cast<const char*>(bytes.data()),
                         bytes.size()),
        default_allow_file_access_);
    if (!request.has_value()) {
      return WriteResponse(output_, ErrorHeader(request.error()), {});
    }

    auto image = Capture(*request);
    if (!image.has_value()) {
      return WriteResponse(output_, ErrorHeader(image.error()), {});
    }

    base::DictValue header;
    header.Set("ok", true);
    header.Set("bytes", static_cast<int>(image->size()));

    // Writing the file here rather than shipping the bytes back is worth a
    // whole round trip of a few hundred kilobytes when the caller only wanted
    // it on disk.
    if (!request->path.empty()) {
      const base::FilePath path = base::FilePath::FromUTF8Unsafe(request->path);
      if (!base::WriteFile(path, *image)) {
        return WriteResponse(
            output_, ErrorHeader("could not write " + request->path), {});
      }
      header.Set("path", request->path);
      return WriteResponse(output_, std::move(header), {});
    }

    return WriteResponse(output_, std::move(header), *image);
  }

  const base::raw_ref<ShotRuntime> runtime_;
  base::File output_;
  const bool default_allow_file_access_;
  base::OnceClosure quit_;
  base::OneShotTimer idle_timer_;
  // Which half of the idle sequence has already run since the last request.
  bool purged_ = false;
  int exit_code_ = kSuccessExitCode;
};

}  // namespace

int RunServer(ShotRuntime& runtime, bool default_allow_file_access) {
  base::File input = StandardInput();
  base::File output = StandardOutput();
  if (!input.IsValid() || !output.IsValid()) {
    LOG(ERROR) << "shot: --serve needs stdin and stdout";
    return kUsageExitCode;
  }

  base::RunLoop run_loop;
  RequestHandler handler(runtime, std::move(output), default_allow_file_access,
                         run_loop.QuitClosure());

  RequestReader reader(
      std::move(input), base::SingleThreadTaskRunner::GetCurrentDefault(),
      base::BindRepeating(&RequestHandler::OnRequest,
                          base::Unretained(&handler)),
      base::BindOnce(&RequestHandler::OnStreamEnd, base::Unretained(&handler)));
  base::DelegateSimpleThread thread(&reader, "ShotRequests");
  thread.Start();

  run_loop.Run();

  // The reader is joinable rather than detached because there is exactly one
  // way for the loop above to stop that the reader has not already stopped for
  // itself -- stdout breaking while stdin stays open -- and a supervisor whose
  // read end is gone has closed the write end too. So the join returns; it is
  // the reader noticing what the writer noticed a moment earlier.
  thread.Join();
  return handler.exit_code();
}

}  // namespace shot
