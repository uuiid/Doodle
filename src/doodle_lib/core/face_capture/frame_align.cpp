//
// Created by TD on 2026-11-6.
//

#include "frame_align.h"

#include <doodle_core/exception/exception.h>
#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/json_warp.h>

#include <doodle_lib/core/file_sys.h>
#include <doodle_lib/logger/logger.h>

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <fstream>

namespace doodle::face_capture {

void to_json(nlohmann::json& nlohmann_json_j, const camera_info& nlohmann_json_t) {
  nlohmann_json_j["index"]   = nlohmann_json_t.index_;
  nlohmann_json_j["name"]    = nlohmann_json_t.name_;
  nlohmann_json_j["backend"] = nlohmann_json_t.backend_;
  nlohmann_json_j["width"]   = nlohmann_json_t.width_;
  nlohmann_json_j["height"]  = nlohmann_json_t.height_;
}

void from_json(const nlohmann::json& nlohmann_json_j, camera_info& nlohmann_json_t) {
  if (nlohmann_json_j.contains("index")) nlohmann_json_j.at("index").get_to(nlohmann_json_t.index_);
  if (nlohmann_json_j.contains("name")) nlohmann_json_j.at("name").get_to(nlohmann_json_t.name_);
  if (nlohmann_json_j.contains("backend")) nlohmann_json_j.at("backend").get_to(nlohmann_json_t.backend_);
  if (nlohmann_json_j.contains("width")) nlohmann_json_j.at("width").get_to(nlohmann_json_t.width_);
  if (nlohmann_json_j.contains("height")) nlohmann_json_j.at("height").get_to(nlohmann_json_t.height_);
}

void to_json(nlohmann::json& nlohmann_json_j, const capture_option& nlohmann_json_t) {
  nlohmann_json_j["device"] = nlohmann_json_t.device_index_;
  nlohmann_json_j["width"]  = nlohmann_json_t.width_;
  nlohmann_json_j["height"] = nlohmann_json_t.height_;
}

void from_json(const nlohmann::json& nlohmann_json_j, capture_option& nlohmann_json_t) {
  if (nlohmann_json_j.contains("device")) nlohmann_json_j.at("device").get_to(nlohmann_json_t.device_index_);
  if (nlohmann_json_j.contains("width")) nlohmann_json_j.at("width").get_to(nlohmann_json_t.width_);
  if (nlohmann_json_j.contains("height")) nlohmann_json_j.at("height").get_to(nlohmann_json_t.height_);
}

void to_json(nlohmann::json& nlohmann_json_j, const capture_status& nlohmann_json_t) {
  nlohmann_json_j["state"]           = nlohmann_json_t.state_;
  nlohmann_json_j["width"]           = nlohmann_json_t.width_;
  nlohmann_json_j["height"]          = nlohmann_json_t.height_;
  nlohmann_json_j["fps"]             = nlohmann_json_t.fps_;
  nlohmann_json_j["frames"]          = nlohmann_json_t.frames_;
  nlohmann_json_j["raw_frames"]      = nlohmann_json_t.raw_frames_;
  nlohmann_json_j["inserted_frames"] = nlohmann_json_t.inserted_frames_;
  nlohmann_json_j["source_fps"]      = nlohmann_json_t.source_fps_;
  nlohmann_json_j["duration_ms"]     = nlohmann_json_t.duration_ms_;
  nlohmann_json_j["truncated"]       = nlohmann_json_t.truncated_;
}

}  // namespace doodle::face_capture
