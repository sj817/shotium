// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "shot/shot_request.h"

#include <string_view>

#include "base/json/json_reader.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "net/http/http_util.h"
#include "shot/shot_capture_context.h"

namespace shot {
namespace {

// Viewport and output bounds. The upper one is not a policy about what is
// reasonable, it is where an int32 pixel count times four bytes stops being
// allocatable.
constexpr int kMinimumDimension = 1;
constexpr int kMaximumDimension = 32767;
constexpr double kMinimumScale = 0.01;
constexpr double kMaximumScale = 8.0;

// Reads an int that must be present as a number, not as a string. Being strict
// here is deliberate: a JS caller that sends "800" instead of 800 has a bug,
// and silently accepting it moves the discovery of that bug into the pixels.
base::expected<std::optional<int>, std::string> ReadInt(
    const base::DictValue& dict,
    std::string_view key) {
  const base::Value* value = dict.Find(key);
  if (!value) {
    return std::optional<int>();
  }
  if (!value->is_int()) {
    return base::unexpected(base::StrCat({key, " must be a number"}));
  }
  return std::optional<int>(value->GetInt());
}

base::expected<std::optional<double>, std::string> ReadDouble(
    const base::DictValue& dict,
    std::string_view key) {
  const base::Value* value = dict.Find(key);
  if (!value) {
    return std::optional<double>();
  }
  if (!value->is_double() && !value->is_int()) {
    return base::unexpected(base::StrCat({key, " must be a number"}));
  }
  return std::optional<double>(value->GetDouble());
}

base::expected<std::optional<bool>, std::string> ReadBool(
    const base::DictValue& dict,
    std::string_view key) {
  const base::Value* value = dict.Find(key);
  if (!value) {
    return std::optional<bool>();
  }
  if (!value->is_bool()) {
    return base::unexpected(base::StrCat({key, " must be a boolean"}));
  }
  return std::optional<bool>(value->GetBool());
}

base::expected<std::optional<std::string>, std::string> ReadString(
    const base::DictValue& dict,
    std::string_view key) {
  const base::Value* value = dict.Find(key);
  if (!value) {
    return std::optional<std::string>();
  }
  if (!value->is_string()) {
    return base::unexpected(base::StrCat({key, " must be a string"}));
  }
  return std::optional<std::string>(value->GetString());
}

std::string OutOfRange(std::string_view key, int low, int high) {
  return base::StrCat({key, " must be between ", base::NumberToString(low),
                       " and ", base::NumberToString(high)});
}

}  // namespace

base::expected<ScreenshotRequest, std::string> ParseScreenshotRequest(
    std::string_view json,
    bool default_allow_file_access) {
  std::optional<base::DictValue> parsed =
      base::JSONReader::ReadDict(json, base::JSON_PARSE_RFC);
  if (!parsed) {
    return base::unexpected("request is not a JSON object");
  }
  const base::DictValue& dict = *parsed;

  if (dict.Find("pngCompression")) {
    return base::unexpected(
        "pngCompression was removed; PNG always uses the fast encoder");
  }

  ScreenshotRequest request;

  auto file = ReadString(dict, "file");
  if (!file.has_value()) {
    return base::unexpected(file.error());
  }
  if (!file->has_value() || (*file)->empty()) {
    return base::unexpected("file is required");
  }
  request.file = **file;

  auto type = ReadString(dict, "type");
  if (!type.has_value()) {
    return base::unexpected(type.error());
  }
  if (type->has_value()) {
    const std::string& value = **type;
    if (value != "png" && value != "jpeg" && value != "webp") {
      return base::unexpected("type must be one of png, jpeg, webp");
    }
    request.type = value;
  }

  auto full_page = ReadBool(dict, "fullPage");
  if (!full_page.has_value()) {
    return base::unexpected(full_page.error());
  }
  request.full_page = full_page->value_or(false);

  auto selector = ReadString(dict, "selector");
  if (!selector.has_value()) {
    return base::unexpected(selector.error());
  }
  request.selector = selector->value_or(std::string());

  auto quality = ReadInt(dict, "quality");
  if (!quality.has_value()) {
    return base::unexpected(quality.error());
  }
  if (quality->has_value()) {
    if (**quality < 1 || **quality > 100) {
      return base::unexpected(OutOfRange("quality", 1, 100));
    }
    request.quality = **quality;
  }

  auto scale = ReadDouble(dict, "scale");
  if (!scale.has_value()) {
    return base::unexpected(scale.error());
  }
  if (scale->has_value()) {
    if (**scale < kMinimumScale || **scale > kMaximumScale) {
      return base::unexpected("scale must be between 0.01 and 8");
    }
    request.scale = **scale;
  }

  auto omit_background = ReadBool(dict, "omitBackground");
  if (!omit_background.has_value()) {
    return base::unexpected(omit_background.error());
  }
  request.omit_background = omit_background->value_or(false);

  auto path = ReadString(dict, "path");
  if (!path.has_value()) {
    return base::unexpected(path.error());
  }
  request.path = path->value_or(std::string());

  auto allow_file_access = ReadBool(dict, "allowFileAccess");
  if (!allow_file_access.has_value()) {
    return base::unexpected(allow_file_access.error());
  }
  request.allow_file_access =
      allow_file_access->value_or(default_allow_file_access);

  auto cache = ReadString(dict, "cache");
  if (!cache.has_value()) {
    return base::unexpected(cache.error());
  }
  if (cache->has_value()) {
    int ignored = 0;
    if (!CacheModeToLoadFlags(**cache, &ignored)) {
      return base::unexpected(
          "cache must be one of default, reload, no-store, only-if-cached");
    }
    request.cache = **cache;
  }

  if (const base::Value* headers = dict.Find("headers")) {
    if (!headers->is_dict()) {
      return base::unexpected("headers must be an object");
    }
    for (const auto [name, value] : headers->GetDict()) {
      if (!value.is_string()) {
        return base::unexpected(
            base::StrCat({"headers.", name, " must be a string"}));
      }
      // Rejected here rather than by //net, which drops an invalid header and
      // carries on: a request that quietly went out without the Authorization
      // the caller supplied would be answered with a login page and
      // photographed as one.
      if (!net::HttpUtil::IsValidHeaderName(name) ||
          !net::HttpUtil::IsValidHeaderValue(value.GetString())) {
        return base::unexpected(
            base::StrCat({"headers.", name, " is not a valid HTTP header"}));
      }
      request.headers[name] = value.GetString();
    }
  }

  for (const auto& [key, field] :
       {std::pair<std::string_view, int*>{"width", &request.width},
        std::pair<std::string_view, int*>{"height", &request.height}}) {
    auto value = ReadInt(dict, key);
    if (!value.has_value()) {
      return base::unexpected(value.error());
    }
    if (value->has_value()) {
      if (**value < kMinimumDimension || **value > kMaximumDimension) {
        return base::unexpected(
            OutOfRange(key, kMinimumDimension, kMaximumDimension));
      }
      *field = **value;
    }
  }

  if (const base::Value* goto_params = dict.Find("pageGotoParams")) {
    if (!goto_params->is_dict()) {
      return base::unexpected("pageGotoParams must be an object");
    }
    const base::DictValue& goto_dict = goto_params->GetDict();

    auto timeout = ReadInt(goto_dict, "timeout");
    if (!timeout.has_value()) {
      return base::unexpected(timeout.error());
    }
    if (timeout->has_value()) {
      if (**timeout < 1) {
        return base::unexpected("pageGotoParams.timeout must be positive");
      }
      request.timeout_ms = **timeout;
    }

    auto wait_until = ReadString(goto_dict, "waitUntil");
    if (!wait_until.has_value()) {
      return base::unexpected(wait_until.error());
    }
    if (wait_until->has_value()) {
      const std::string& value = **wait_until;
      if (value != "load" && value != "networkidle") {
        return base::unexpected(
            "pageGotoParams.waitUntil must be one of load, networkidle");
      }
      request.wait_until = value;
    }
  }

  if (const base::Value* clip = dict.Find("clip")) {
    if (!clip->is_dict()) {
      return base::unexpected("clip must be an object");
    }
    const base::DictValue& clip_dict = clip->GetDict();
    Clip parsed_clip;
    for (const auto& [key, field] :
         {std::pair<std::string_view, int*>{"x", &parsed_clip.x},
          std::pair<std::string_view, int*>{"y", &parsed_clip.y},
          std::pair<std::string_view, int*>{"width", &parsed_clip.width},
          std::pair<std::string_view, int*>{"height", &parsed_clip.height}}) {
      auto value = ReadInt(clip_dict, key);
      if (!value.has_value()) {
        return base::unexpected(base::StrCat({"clip.", value.error()}));
      }
      if (!value->has_value()) {
        return base::unexpected(base::StrCat({"clip.", key, " is required"}));
      }
      *field = **value;
    }
    if (parsed_clip.width < kMinimumDimension ||
        parsed_clip.height < kMinimumDimension) {
      return base::unexpected("clip.width and clip.height must be positive");
    }
    request.clip = parsed_clip;
  }

  // Combinations that parse but cannot be honoured. Saying so beats the
  // alternatives: silently dropping the field, or writing a JPEG whose
  // "transparent" areas came out black.
  if (request.quality.has_value() && request.type == "png") {
    return base::unexpected("quality applies to jpeg and webp; png is lossless");
  }
  if (request.omit_background && request.type == "jpeg") {
    return base::unexpected(
        "omitBackground needs an alpha channel and jpeg has none; use png or "
        "webp");
  }
  // selector, clip and fullPage each name the region to capture, and they name
  // different ones. Picking a winner would mean the caller cannot tell which
  // field was honoured by looking at the result.
  if (!request.selector.empty() && request.clip.has_value()) {
    return base::unexpected("selector and clip both choose a region; use one");
  }
  if (request.full_page && request.clip.has_value()) {
    return base::unexpected("fullPage and clip both choose a region; use one");
  }
  if (request.full_page && !request.selector.empty()) {
    return base::unexpected(
        "fullPage and selector both choose a region; use one");
  }

  return request;
}

}  // namespace shot
