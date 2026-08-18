// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_server.h"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/span.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/json/json_writer.h"
#include "base/logging.h"
#include "base/numerics/byte_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "shot/shot_capture.h"
#include "shot/shot_options.h"
#include "shot/shot_request.h"

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

}  // namespace

int RunServer() {
  base::File input = StandardInput();
  base::File output = StandardOutput();
  if (!input.IsValid() || !output.IsValid()) {
    LOG(ERROR) << "shot: --serve needs stdin and stdout";
    return kUsageExitCode;
  }

  std::vector<uint8_t> request_bytes;
  while (true) {
    std::array<uint8_t, 4> length_bytes = {};
    bool at_eof = false;
    if (!ReadExactly(input, length_bytes, &at_eof)) {
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

    request_bytes.assign(length, 0);
    if (!ReadExactly(input, request_bytes, &at_eof)) {
      LOG(ERROR) << "shot: the request stream ended inside a frame";
      return kCaptureExitCode;
    }

    auto request = ParseScreenshotRequest(
        std::string_view(reinterpret_cast<const char*>(request_bytes.data()),
                         request_bytes.size()));
    if (!request.has_value()) {
      if (!WriteResponse(output, ErrorHeader(request.error()), {})) {
        return kCaptureExitCode;
      }
      continue;
    }

    auto image = Capture(*request);
    if (!image.has_value()) {
      if (!WriteResponse(output, ErrorHeader(image.error()), {})) {
        return kCaptureExitCode;
      }
      continue;
    }

    base::DictValue header;
    header.Set("ok", true);
    header.Set("bytes", static_cast<int>(image->size()));

    // Writing the file here rather than shipping the bytes back is worth a
    // whole round trip of a few hundred kilobytes when the caller only wanted
    // it on disk.
    if (!request->path.empty()) {
      const base::FilePath path =
          base::FilePath::FromUTF8Unsafe(request->path);
      if (!base::WriteFile(path, *image)) {
        if (!WriteResponse(
                output, ErrorHeader("could not write " + request->path), {})) {
          return kCaptureExitCode;
        }
        continue;
      }
      header.Set("path", request->path);
      if (!WriteResponse(output, std::move(header), {})) {
        return kCaptureExitCode;
      }
      continue;
    }

    if (!WriteResponse(output, std::move(header), *image)) {
      return kCaptureExitCode;
    }
  }
}

}  // namespace shot
