// Copyright 2026 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#define SHOT_IMPLEMENTATION
#include "shot/shot_api.h"

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "shot/shot_bytes.h"
#include "shot/shot_capture_context.h"
#include "shot/shot_engine.h"
#include "shot/shot_request.h"

static_assert(static_cast<int>(shot::EngineStatus::kOk) == SHOT_OK);
static_assert(static_cast<int>(shot::EngineStatus::kUsage) == SHOT_ERR_USAGE);
static_assert(static_cast<int>(shot::EngineStatus::kCapture) ==
              SHOT_ERR_CAPTURE);
static_assert(static_cast<int>(shot::EngineStatus::kState) == SHOT_ERR_STATE);

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

}  // namespace
}  // namespace shot

struct shot_engine {
  std::shared_ptr<shot::EngineService> service;
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
    shot::Deliver(out_error,
                  "shot_engine_create needs somewhere to put the "
                  "engine");
    return SHOT_ERR_USAGE;
  }

  base::DictValue dict;
  if (options_json && *options_json) {
    auto parsed =
        base::JSONReader::ReadDict(options_json, base::JSON_PARSE_RFC);
    if (!parsed) {
      shot::Deliver(out_error, "options must be a JSON object");
      return SHOT_ERR_USAGE;
    }
    dict = std::move(*parsed);
  }
  auto options = shot::ReadEngineOptions(dict);
  if (!options.has_value()) {
    shot::Deliver(out_error, options.error());
    return SHOT_ERR_USAGE;
  }
  auto service = shot::EngineService::Create(std::move(*options));
  if (!service.has_value()) {
    shot::Deliver(out_error, service.error());
    return SHOT_ERR_STATE;
  }
  *out_engine = new shot_engine{std::move(*service)};
  return SHOT_OK;
}

void shot_engine_destroy(shot_engine* engine) {
  if (!engine) {
    return;
  }
  engine->service->Stop();
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

  auto request = shot::ParseScreenshotRequest(
      request_json, engine->service->default_allow_file_access());
  if (!request.has_value()) {
    shot::Deliver(out_error, request.error());
    return SHOT_ERR_USAGE;
  }
  auto result = engine->service->CaptureSync(std::move(*request), false);
  if (result.stats) {
    shot::Deliver(out_stats, shot::StatsToJson(*result.stats));
  }
  if (result.status != shot::EngineStatus::kOk) {
    shot::Deliver(out_error, result.error);
    return static_cast<shot_status>(result.status);
  }
  *out_image = shot::MakeImage(std::move(result.image));
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

  auto request = shot::ParseScreenshotRequest(
      request_json, engine->service->default_allow_file_access());
  if (!request.has_value()) {
    shot::Deliver(out_error, request.error());
    return SHOT_ERR_USAGE;
  }
  auto result = engine->service->CaptureSync(std::move(*request), true);
  if (result.stats) {
    shot::Deliver(out_stats, shot::StatsToJson(*result.stats));
  }
  if (result.status != shot::EngineStatus::kOk) {
    shot::Deliver(out_error, result.error);
    return static_cast<shot_status>(result.status);
  }
  auto tiles = std::move(result.tiles);
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
  base::JSONWriter::Write(engine->service->Status(), &json);
  *out_json = shot::MakeMessage(json);
  return SHOT_OK;
}

// Shared cache adapter: the C seam retains its synchronous JSON contract.
static shot_status CacheCall(shot_engine* engine,
                             const char* options_json,
                             bool clearing,
                             shot_buffer** out_json,
                             shot_buffer** out_error) {
  shot::Clear(out_json);
  shot::Clear(out_error);
  if (!out_json) {
    shot::Deliver(out_error,
                  clearing
                      ? "shot_cache_clear needs somewhere to put the result"
                      : "shot_cache_list needs somewhere to put the list");
    return SHOT_ERR_USAGE;
  }
  if (!options_json || !*options_json) {
    shot::Deliver(out_error, "cacheDir is required");
    return SHOT_ERR_USAGE;
  }
  auto dict = base::JSONReader::ReadDict(options_json, base::JSON_PARSE_RFC);
  if (!dict) {
    shot::Deliver(out_error, "options must be a JSON object");
    return SHOT_ERR_USAGE;
  }
  auto options = shot::ReadCacheOptions(*dict);
  if (!options.has_value()) {
    shot::Deliver(out_error, options.error());
    return SHOT_ERR_USAGE;
  }
  auto result = engine
                    ? engine->service->CacheSync(std::move(*options), clearing)
                    : shot::CacheWithoutEngine(std::move(*options), clearing);
  if (result.status != shot::EngineStatus::kOk) {
    shot::Deliver(out_error, result.error);
    return static_cast<shot_status>(result.status);
  }
  std::string json;
  base::JSONWriter::Write(result.value, &json);
  *out_json = shot::MakeMessage(json);
  return SHOT_OK;
}
shot_status shot_cache_list(shot_engine* engine,
                            const char* options_json,
                            shot_buffer** out_json,
                            shot_buffer** out_error) {
  return CacheCall(engine, options_json, false, out_json, out_error);
}
shot_status shot_cache_clear(shot_engine* engine,
                             const char* options_json,
                             shot_buffer** out_json,
                             shot_buffer** out_error) {
  return CacheCall(engine, options_json, true, out_json, out_error);
}
void shot_engine_purge(shot_engine* engine, int32_t release_working_set) {
  if (engine) {
    engine->service->Purge(release_working_set != 0);
  }
}
}  // extern "C"
