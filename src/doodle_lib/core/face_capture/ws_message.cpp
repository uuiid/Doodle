//
// Created by TD on 2026-11-6.
//

#include "ws_message.h"

#include <doodle_core/lib_warp/json_warp.h>

#include <fmt/format.h>

namespace doodle::face_capture {

std::vector<std::uint8_t> ws_message::payload() const { return {}; }

nlohmann::json ws_hello::to_json() const {
  nlohmann::json l_json{};
  l_json["type"]       = "hello";
  l_json["session_id"] = fmt::format("{}", session_id_);
  l_json["state"]      = state_;
  l_json["width"]      = width_;
  l_json["height"]     = height_;
  l_json["fps"]        = k_fps;
  return l_json;
}

nlohmann::json ws_avc_init::to_json() const {
  nlohmann::json l_json{};
  l_json["type"]      = "avc_init";
  l_json["bytes"]     = init_segment_.size();
  l_json["seq_begin"] = 0;
  l_json["seq_end"]   = 0;
  return l_json;
}

nlohmann::json ws_avc_fragment::to_json() const {
  nlohmann::json l_json{};
  l_json["type"]     = "avc_fragment";
  l_json["seq"]      = fragment_.seq_begin_;
  l_json["keyframe"] = fragment_.key_;
  l_json["size"]     = fragment_.data_.size();
  return l_json;
}

}  // namespace doodle::face_capture
