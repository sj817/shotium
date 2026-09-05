// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_options.h"

#include <cstdio>
#include <optional>
#include <string_view>
#include <utility>

#include "base/files/file_util.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/stringprintf.h"
#include "build/build_config.h"
#include "net/base/filename_util.h"
#include "shot/shot_request.h"
#include "url/url_constants.h"

#if BUILDFLAG(IS_WIN)
#include <fcntl.h>
#include <io.h>
#endif

namespace shot {
namespace {

constexpr int kMinimumViewportDimension = 1;
constexpr int kMaximumViewportDimension = 16384;
constexpr double kMinimumScale = 0.01;
constexpr double kMaximumScale = 8.0;
constexpr int kMinimumTimeoutMilliseconds = 1;
constexpr int kMaximumTimeoutMilliseconds = 600000;
constexpr size_t kMaximumStdinBytes = 64 * 1024 * 1024;

bool IsPassThroughFlag(std::string_view argument) {
  // These are bootstrap escape hatches, not part of Shot's long-term API.
  return argument == "--no-sandbox" || argument == "--disable-gpu" ||
         argument == "--enable-gpu" || argument == "--enable-logging" ||
         argument.starts_with("--enable-logging=") ||
         argument.starts_with("--v=") || argument.starts_with("--vmodule=") ||
         argument.starts_with("--single-process");
}

std::optional<std::string> ConsumeValue(
    const std::vector<std::string>& argv,
    size_t* index,
    std::string_view argument,
    std::string_view long_name,
    std::optional<std::string_view> short_name = std::nullopt) {
  const std::string long_switch = "--" + std::string(long_name);
  const std::string long_prefix = long_switch + "=";

  if (argument.starts_with(long_prefix)) {
    return std::string(argument.substr(long_prefix.size()));
  }
  if (argument == long_switch ||
      (short_name.has_value() && argument == *short_name)) {
    if (*index + 1 >= argv.size()) {
      return std::string();
    }
    return argv[++*index];
  }
  return std::nullopt;
}

base::expected<int, std::string> ParseBoundedInteger(std::string_view name,
                                                     std::string_view value,
                                                     int minimum,
                                                     int maximum) {
  int parsed = 0;
  if (!base::StringToInt(value, &parsed) || parsed < minimum ||
      parsed > maximum) {
    return base::unexpected(
        "--" + std::string(name) +
        base::StringPrintf(" must be an integer in [%d, %d]", minimum,
                           maximum));
  }
  return parsed;
}

base::expected<std::string, std::string> ReadStdin() {
#if BUILDFLAG(IS_WIN)
  if (_setmode(_fileno(stdin), _O_BINARY) == -1) {
    return base::unexpected("failed to put stdin in binary mode");
  }
#endif

  // Reading through a raw buffer trips -Wunsafe-buffer-usage-in-libc-call.
  // This also reports the size limit and a read failure as the same result, so
  // the two are distinguished by whether the limit was actually reached.
  std::string input;
  if (!base::ReadStreamToStringWithMaxSize(stdin, kMaximumStdinBytes, &input)) {
    if (input.size() >= kMaximumStdinBytes) {
      return base::unexpected("stdin exceeds the 64 MiB first-version limit");
    }
    return base::unexpected("failed while reading stdin");
  }
  // The value and error types are both std::string, so the conversion has to
  // say which one this is.
  return base::ok(std::move(input));
}

base::expected<GURL, std::string> ResolveFileInput(
    const base::FilePath& input_path) {
  if (!base::PathExists(input_path)) {
    return base::unexpected("input file does not exist: " +
                            input_path.AsUTF8Unsafe());
  }
  if (base::DirectoryExists(input_path)) {
    return base::unexpected("input path is a directory: " +
                            input_path.AsUTF8Unsafe());
  }

  const base::FilePath absolute_path = base::MakeAbsoluteFilePath(input_path);
  if (absolute_path.empty()) {
    return base::unexpected("failed to make input path absolute: " +
                            input_path.AsUTF8Unsafe());
  }

  GURL file_url = net::FilePathToFileURL(absolute_path);
  if (!file_url.is_valid()) {
    return base::unexpected("failed to convert input path to a file URL: " +
                            absolute_path.AsUTF8Unsafe());
  }
  return file_url;
}

bool IsSupportedUrl(const GURL& url) {
  return url.SchemeIsHTTPOrHTTPS() || url.SchemeIs(url::kFileScheme) ||
         url.SchemeIs(url::kDataScheme);
}

}  // namespace

base::expected<ShotOptions, std::string> ParseShotOptions(
    const std::vector<std::string>& argv) {
  ShotOptions options;
  bool positional_only = false;
  bool has_positional_input = false;
  bool has_file_input = false;

  for (size_t i = 1; i < argv.size(); ++i) {
    const std::string_view argument(argv[i]);

    if (!positional_only && argument == "--") {
      positional_only = true;
      continue;
    }
    if (!positional_only && (argument == "--help" || argument == "-h")) {
      options.show_help = true;
      continue;
    }
    if (!positional_only && argument == "--stdin") {
      options.read_stdin = true;
      continue;
    }
    if (!positional_only && argument == "--serve") {
      options.serve = true;
      continue;
    }
    if (!positional_only && argument == "--allow-file-access") {
      options.allow_file_access = true;
      continue;
    }
    if (!positional_only && argument == "--full-page") {
      options.full_page = true;
      continue;
    }
    if (!positional_only && argument == "--omit-background") {
      options.omit_background = true;
      continue;
    }
    // Recognised here only so it is not an error; main() reads it straight off
    // base::CommandLine, because the log level has to be set before anything
    // this parser could report.
    if (!positional_only && argument == "--verbose") {
      options.verbose = true;
      continue;
    }

    if (!positional_only) {
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "width")) {
        if (value->empty()) {
          return base::unexpected("--width requires a value");
        }
        auto parsed =
            ParseBoundedInteger("width", *value, kMinimumViewportDimension,
                                kMaximumViewportDimension);
        if (!parsed.has_value()) {
          return base::unexpected(parsed.error());
        }
        options.width = parsed.value();
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "height")) {
        if (value->empty()) {
          return base::unexpected("--height requires a value");
        }
        auto parsed =
            ParseBoundedInteger("height", *value, kMinimumViewportDimension,
                                kMaximumViewportDimension);
        if (!parsed.has_value()) {
          return base::unexpected(parsed.error());
        }
        options.height = parsed.value();
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "output", "-o")) {
        if (value->empty()) {
          return base::unexpected("--output requires a value");
        }
        options.output_path = base::FilePath::FromUTF8Unsafe(*value);
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "tile-height")) {
        int parsed = 0;
        if (!base::StringToInt(*value, &parsed) || parsed < 1 ||
            parsed > kMaximumTileHeight) {
          return base::unexpected(
              "--tile-height must be an integer from 1 to " +
              base::NumberToString(kMaximumTileHeight));
        }
        options.tile_height = parsed;
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "timeout-ms")) {
        if (value->empty()) {
          return base::unexpected("--timeout-ms requires a value");
        }
        auto parsed = ParseBoundedInteger("timeout-ms", *value,
                                          kMinimumTimeoutMilliseconds,
                                          kMaximumTimeoutMilliseconds);
        if (!parsed.has_value()) {
          return base::unexpected(parsed.error());
        }
        options.timeout = base::Milliseconds(parsed.value());
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "timeout")) {
        if (value->empty()) {
          return base::unexpected("--timeout requires a millisecond value");
        }
        auto parsed =
            ParseBoundedInteger("timeout", *value, kMinimumTimeoutMilliseconds,
                                kMaximumTimeoutMilliseconds);
        if (!parsed.has_value()) {
          return base::unexpected(parsed.error());
        }
        options.timeout = base::Milliseconds(parsed.value());
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "scale")) {
        if (value->empty() || !base::StringToDouble(*value, &options.scale) ||
            options.scale < kMinimumScale || options.scale > kMaximumScale) {
          return base::unexpected(base::StringPrintf(
              "--scale must be a number in [%g, %g]", kMinimumScale,
              kMaximumScale));
        }
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "selector")) {
        if (value->empty()) {
          return base::unexpected("--selector requires a value");
        }
        options.selector = std::move(*value);
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "type")) {
        if (*value != "png" && *value != "jpeg" && *value != "webp") {
          return base::unexpected("--type must be one of png, jpeg, webp");
        }
        options.type = std::move(*value);
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "quality")) {
        if (value->empty()) {
          return base::unexpected("--quality requires a value");
        }
        auto parsed = ParseBoundedInteger("quality", *value, 1, 100);
        if (!parsed.has_value()) {
          return base::unexpected(parsed.error());
        }
        options.quality = parsed.value();
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "wait-until")) {
        if (*value != "load" && *value != "networkidle") {
          return base::unexpected(
              "--wait-until must be one of load, networkidle");
        }
        options.wait_until = std::move(*value);
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "cache-dir")) {
        if (value->empty()) {
          return base::unexpected("--cache-dir requires a value");
        }
        options.cache_dir = base::FilePath::FromUTF8Unsafe(*value);
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "cache-max-bytes")) {
        // Reachable from the library since 0.3 as `cacheMaxBytes`, and here
        // for the same reason: the field existed and nothing could set it, so
        // the backend's own default -- a fraction of the volume's free space
        // -- was the only size a cache could ever have.
        int64_t bytes = 0;
        if (!base::StringToInt64(*value, &bytes) || bytes < 0) {
          return base::unexpected(
              "--cache-max-bytes must be a non-negative number");
        }
        options.cache_max_bytes = static_cast<int>(bytes);
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "user-agent")) {
        if (value->empty()) {
          return base::unexpected("--user-agent requires a value");
        }
        options.user_agent = std::move(*value);
        continue;
      }
      if (std::optional<std::string> value =
              ConsumeValue(argv, &i, argument, "file")) {
        if (value->empty()) {
          return base::unexpected("--file requires a value");
        }
        if (has_file_input) {
          return base::unexpected("--file may only be specified once");
        }
        options.input = std::move(*value);
        options.force_file_input = true;
        has_file_input = true;
        continue;
      }
      if (IsPassThroughFlag(argument)) {
        continue;
      }
      if (argument.starts_with('-')) {
        return base::unexpected("unknown option: " + std::string(argument));
      }
    }

    if (has_positional_input) {
      return base::unexpected("only one positional input is supported");
    }
    options.input = argument;
    has_positional_input = true;
  }

  if (options.show_help) {
    return options;
  }

  const int input_mode_count = static_cast<int>(options.read_stdin) +
                               static_cast<int>(has_file_input) +
                               static_cast<int>(has_positional_input);
  // --serve takes its input from the request stream, so an input here would be
  // ambiguous rather than redundant: there would be no answer to which one the
  // first request meant.
  if (options.serve) {
    if (input_mode_count != 0) {
      return base::unexpected("--serve takes no input; requests arrive on stdin");
    }
    return options;
  }
  if (input_mode_count != 1) {
    return base::unexpected(
        "specify exactly one URL/path, --file PATH, or --stdin");
  }
  if (options.output_path.empty()) {
    return base::unexpected("output path must not be empty");
  }
  return options;
}

base::expected<PreparedShot, std::string> PrepareShot(ShotOptions options) {
  PreparedShot prepared;
  prepared.options = std::move(options);

  if (prepared.options.read_stdin) {
    auto html = ReadStdin();
    if (!html.has_value()) {
      return base::unexpected(html.error());
    }
    if (!prepared.stdin_temp_dir.CreateUniqueTempDir(
            FILE_PATH_LITERAL("shot-stdin-"))) {
      return base::unexpected("failed to create a temporary stdin directory");
    }
    const base::FilePath html_path = prepared.stdin_temp_dir.GetPath().Append(
        FILE_PATH_LITERAL("stdin.html"));
    if (!base::WriteFile(html_path, html.value())) {
      return base::unexpected("failed to write stdin HTML to a temporary file");
    }
    prepared.target_url = net::FilePathToFileURL(html_path);
    if (!prepared.target_url.is_valid()) {
      return base::unexpected("failed to create the stdin HTML file URL");
    }
    return std::move(prepared);
  }

  const base::FilePath possible_path =
      base::FilePath::FromUTF8Unsafe(prepared.options.input);
  if (prepared.options.force_file_input || base::PathExists(possible_path) ||
      possible_path.IsAbsolute()) {
    auto file_url = ResolveFileInput(possible_path);
    if (!file_url.has_value()) {
      return base::unexpected(file_url.error());
    }
    prepared.target_url = std::move(file_url.value());
    return std::move(prepared);
  }

  GURL url(prepared.options.input);
  if (!url.is_valid() || !url.has_scheme()) {
    return base::unexpected(
        "input is neither a valid URL nor an existing file: " +
        prepared.options.input);
  }
  if (!IsSupportedUrl(url)) {
    return base::unexpected(std::string("unsupported URL scheme '") +
                            std::string(url.scheme()) +
                            "' (allowed: http, https, file, data)");
  }
  prepared.target_url = std::move(url);
  return std::move(prepared);
}

std::string GetUsage() {
  return R"(Usage:
  shot URL_OR_PATH [options]
  shot --file PATH [options]
  shot --stdin [options]

Options:
  --width N             Viewport width in CSS pixels (default: 1280)
  --height N            Viewport height in CSS pixels (default: 720)
  --scale N             Device scale factor, 0.01-8 (default: 1)
  --full-page           Capture the whole document, not just the viewport
  --selector CSS        Capture only the first element matching CSS
  --tile-height N       Write the capture as tiles of at most N CSS pixels
                        each, numbered into --output: page-{n}.png, or
                        page-1.png, page-2.png ... when {n} is not given
  --type TYPE           png, jpeg or webp (default: png)
  --quality N           1-100, jpeg and webp only (default: 90)
  --omit-background     Keep the alpha channel instead of painting white
  --wait-until WHEN     load or networkidle (default: load)
  --output PATH, -o PATH
                        Output path (default: screenshot.png)
  --timeout-ms N        Load timeout in milliseconds (default: 30000)
  --timeout N           Alias for --timeout-ms
  --serve               Resident worker: read length-prefixed JSON requests
                        from stdin and write results to stdout
  --allow-file-access   Let --serve requests that do not say load file://
                        subresources. A request may still set allowFileAccess
                        either way; this only changes what silence means. The
                        command line already allows it and ignores this flag.
  --cache-dir PATH      HTTP disk cache directory; without it nothing is cached
  --cache-max-bytes N   Ceiling on that directory; 0 (the default) lets the
                        backend size itself from the volume's free space
  --user-agent STRING   Override the User-Agent sent and reported
  --verbose             Log every subresource request and its outcome
  --help, -h            Show this help

http, https, file and data URLs are accepted, as are local paths. Several
processes may share one cache directory: the simple backend takes no lock
across processes, and each keeps its own index of what is in it. The entries
carry checksums, so the worst a disagreement costs is an index rebuild -- but
one directory per worker still writes fewer of them.
)";
}

}  // namespace shot
