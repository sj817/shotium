// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "base/at_exit.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/containers/span.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/logging/logging_settings.h"
#include "build/build_config.h"
#include "shot/shot_capture.h"
#include "shot/shot_options.h"
#include "shot/shot_request.h"
#include "shot/shot_runtime.h"
#include "shot/shot_server.h"

#if BUILDFLAG(IS_POSIX)
#include <csignal>
#endif

namespace {

// shot turns markup into pixels, in this process, on this thread.
//
// It used to start with content::ContentMain: a browser main loop, a render
// process, a sandbox, an aura WindowTreeHost, a ui::Compositor and viz. All of
// that existed to be a *browser*; none of it is required to turn markup into
// pixels. See docs/cut-progress.md section 14, and
// core/svg/graphics/isolated_svg_document_host.cc for the same shape inside
// blink itself.
//
// There are two ways in. The command line renders one document and exits, which
// is what this was. --serve keeps the process alive and answers requests off
// stdin; blink is a process-wide singleton, so staying resident is the only way
// to render more than one document without paying to start a process each time.
// Both go through Capture(), so neither can drift from the other.
//
// Node does not use either of them any more. It loads shot_c into its own
// process through a Node-API addon, which is the same Capture() again --
// node_check.cjs asserts the images match byte for byte. --serve is for a
// caller that wants a resident renderer and is not node.
int Main(int argc, const char** argv) {
  base::AtExitManager at_exit;
  base::CommandLine::Init(argc, argv);

#if BUILDFLAG(IS_POSIX)
  // Ignore SIGPIPE, so that writing to a pipe nobody is reading returns EPIPE
  // rather than killing the process outright.
  //
  // Every write in this program checks its return value and has something to
  // do about a failure: --serve stops reading requests and exits with a code,
  // one-shot mode reports that it could not write the image. None of that runs
  // if the default disposition gets there first, which on POSIX it does -- so
  // on Linux and macOS the whole "stdout is gone" path was unreachable, and a
  // worker whose supervisor died mid-response reached the pool as killed by
  // signal 13 rather than as a process that noticed and stopped.
  //
  // //content does this in the same place and for the same stated reason
  // (content_main.cc, SetupSignalHandlers: "Always ignore SIGPIPE. We check
  // the return value of write."), which is where this process would have
  // inherited it from had it kept the browser. Windows needs no equivalent: a
  // broken pipe is an error out of WriteFile, which is what this code already
  // expects everywhere.
  CHECK_NE(SIG_ERR, signal(SIGPIPE, SIG_IGN));
#endif

  // Logging to stderr, explicitly. On Windows LOG_DEFAULT is
  // LOG_TO_SYSTEM_DEBUG_LOG -- OutputDebugString and nothing else -- so a
  // command-line tool that does not say otherwise throws away everything it
  // logs, including the page's own console errors. That is how a wrong
  // rendering stays silent.
  //
  // It also has to be stderr rather than stdout in --serve mode, where stdout
  // carries the framed protocol and one stray line would desynchronise it.
  //
  // WARNING and above by default so a normal run prints only what went wrong;
  // --verbose adds the per-request load log, which is the trace to read when a
  // subresource does not appear in the picture.
  logging::LoggingSettings log_settings;
  log_settings.logging_dest = logging::LOG_TO_STDERR;
  logging::InitLogging(log_settings);
  logging::SetMinLogLevel(
      base::CommandLine::ForCurrentProcess()->HasSwitch("verbose")
          ? logging::LOGGING_INFO
          : logging::LOGGING_WARNING);

  // argv is a pointer and a separate count -- the one place in the program
  // where that pairing is imposed from outside and cannot be expressed as a
  // container. UNSAFE_BUFFERS is the sanctioned way to say "the bounds are
  // known correct here"; argc is the C runtime's own count of argv.
  const auto argv_span =
      UNSAFE_BUFFERS(base::span(argv, static_cast<size_t>(argc)));
  std::vector<std::string> args(argv_span.begin(), argv_span.end());
  auto parsed = shot::ParseShotOptions(args);
  if (!parsed.has_value()) {
    LOG(ERROR) << "shot: " << parsed.error() << "\n\n" << shot::GetUsage();
    return shot::kUsageExitCode;
  }
  if (parsed->show_help) {
    printf("%s\n", shot::GetUsage().c_str());
    return shot::kSuccessExitCode;
  }

  const bool serve = parsed->serve;

  // Everything blink and //net need, brought up once. In --serve mode it stays
  // up for the life of the process and every request reuses it -- including the
  // HTTP cache, the connection pool and the cookie jar, which is most of what
  // makes a resident worker worth having.
  shot::NetworkConfig network_config;
  network_config.cache_dir = parsed->cache_dir;
  network_config.cache_max_bytes = parsed->cache_max_bytes;
  network_config.user_agent = parsed->user_agent;
  auto runtime = shot::ShotRuntime::Create(network_config);
  if (!runtime.has_value()) {
    LOG(ERROR) << "shot: " << runtime.error();
    return shot::kCaptureExitCode;
  }

  if (serve) {
    return shot::RunServer(*runtime.value(), parsed->allow_file_access);
  }

  // PrepareShot owns the temporary file that makes --stdin navigable, so it
  // has to outlive the render that reads it.
  auto prepared = shot::PrepareShot(std::move(parsed).value());
  if (!prepared.has_value()) {
    LOG(ERROR) << "shot: " << prepared.error();
    return shot::kUsageExitCode;
  }

  shot::ScreenshotRequest request;
  request.file = prepared->target_url.spec();
  request.width = prepared->options.width;
  request.height = prepared->options.height;
  request.scale = prepared->options.scale;
  request.full_page = prepared->options.full_page;
  request.selector = prepared->options.selector;
  request.type = prepared->options.type;
  request.quality = prepared->options.quality;
  request.omit_background = prepared->options.omit_background;
  request.wait_until = prepared->options.wait_until;
  request.timeout_ms =
      static_cast<int>(prepared->options.timeout.InMilliseconds());
  // The command line was pointed at a local file and told to photograph it, so
  // it has already decided that the document may read the files it references.
  // A caller coming in over --serve has not, which is why this is a request
  // field and not a constant.
  request.allow_file_access = true;

  auto image = shot::Capture(request);
  if (!image.has_value()) {
    LOG(ERROR) << "shot: " << image.error();
    return shot::kCaptureExitCode;
  }

  if (!base::WriteFile(prepared->options.output_path, image.value())) {
    LOG(ERROR) << "shot: could not write "
               << prepared->options.output_path.AsUTF8Unsafe();
    return shot::kCaptureExitCode;
  }
  return shot::kSuccessExitCode;
}

}  // namespace

int main(int argc, const char** argv) {
  return Main(argc, argv);
}
