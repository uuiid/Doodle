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

void to_json(nlohmann::json& nlohmann_json_j, const capture_state& nlohmann_json_t) {
  nlohmann_json_j = doodle::face_capture::to_string(nlohmann_json_t);
}

void from_json(const nlohmann::json& nlohmann_json_j, capture_state& nlohmann_json_t) {
  auto l_str = nlohmann_json_j.get<std::string>();
  if (l_str == "idle") {
    nlohmann_json_t = capture_state::idle;
  } else if (l_str == "recording") {
    nlohmann_json_t = capture_state::recording;
  } else if (l_str == "stopping") {
    nlohmann_json_t = capture_state::stopping;
  } else if (l_str == "stopped") {
    nlohmann_json_t = capture_state::stopped;
  } else if (l_str == "previewing") {
    nlohmann_json_t = capture_state::previewing;
  } else {
    throw_exception(doodle_error{fmt::format("未知的采集状态: {}", l_str)});
  }
}

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

void to_json(nlohmann::json& nlohmann_json_j, const gap_range& nlohmann_json_t) {
  nlohmann_json_j["wall_ms_begin"] = nlohmann_json_t.wall_ms_begin_;
  nlohmann_json_j["wall_ms_end"]   = nlohmann_json_t.wall_ms_end_;
  nlohmann_json_j["duration_ms"]   = nlohmann_json_t.duration_ms_;
}

void from_json(const nlohmann::json& nlohmann_json_j, gap_range& nlohmann_json_t) {
  nlohmann_json_j.at("wall_ms_begin").get_to(nlohmann_json_t.wall_ms_begin_);
  nlohmann_json_j.at("wall_ms_end").get_to(nlohmann_json_t.wall_ms_end_);
  nlohmann_json_j.at("duration_ms").get_to(nlohmann_json_t.duration_ms_);
}

void to_json(nlohmann::json& nlohmann_json_j, const align_frame& nlohmann_json_t) {
  nlohmann_json_j["seq"]      = nlohmann_json_t.seq_;
  nlohmann_json_j["orig_seq"] = nlohmann_json_t.orig_seq_;
  nlohmann_json_j["wall_ms"]  = nlohmann_json_t.wall_ms_;
}

void from_json(const nlohmann::json& nlohmann_json_j, align_frame& nlohmann_json_t) {
  nlohmann_json_j.at("seq").get_to(nlohmann_json_t.seq_);
  nlohmann_json_j.at("orig_seq").get_to(nlohmann_json_t.orig_seq_);
  nlohmann_json_j.at("wall_ms").get_to(nlohmann_json_t.wall_ms_);
}

void to_json(nlohmann::json& nlohmann_json_j, const align_record& nlohmann_json_t) {
  nlohmann_json_j["session_id"]      = nlohmann_json_t.session_id_;
  nlohmann_json_j["width"]           = nlohmann_json_t.width_;
  nlohmann_json_j["height"]          = nlohmann_json_t.height_;
  nlohmann_json_j["fps"]             = nlohmann_json_t.fps_;
  nlohmann_json_j["source_fps"]      = nlohmann_json_t.source_fps_;
  nlohmann_json_j["started_at"]      = nlohmann_json_t.started_at_;
  nlohmann_json_j["duration_ms"]     = nlohmann_json_t.duration_ms_;
  nlohmann_json_j["raw_frames"]      = nlohmann_json_t.raw_frames_;
  nlohmann_json_j["inserted_frames"] = nlohmann_json_t.inserted_frames_;
  nlohmann_json_j["frames"]          = nlohmann_json_t.frames_;
  nlohmann_json_j["gaps"]            = nlohmann_json_t.gaps_;
  nlohmann_json_j["truncated"]       = nlohmann_json_t.truncated_;
}

void from_json(const nlohmann::json& nlohmann_json_j, align_record& nlohmann_json_t) {
  nlohmann_json_j.at("session_id").get_to(nlohmann_json_t.session_id_);
  if (nlohmann_json_j.contains("width")) nlohmann_json_j.at("width").get_to(nlohmann_json_t.width_);
  if (nlohmann_json_j.contains("height")) nlohmann_json_j.at("height").get_to(nlohmann_json_t.height_);
  nlohmann_json_t.fps_ = k_fps;
  if (nlohmann_json_j.contains("source_fps")) nlohmann_json_j.at("source_fps").get_to(nlohmann_json_t.source_fps_);
  if (nlohmann_json_j.contains("started_at")) nlohmann_json_j.at("started_at").get_to(nlohmann_json_t.started_at_);
  if (nlohmann_json_j.contains("duration_ms")) nlohmann_json_j.at("duration_ms").get_to(nlohmann_json_t.duration_ms_);
  if (nlohmann_json_j.contains("raw_frames")) nlohmann_json_j.at("raw_frames").get_to(nlohmann_json_t.raw_frames_);
  if (nlohmann_json_j.contains("inserted_frames"))
    nlohmann_json_j.at("inserted_frames").get_to(nlohmann_json_t.inserted_frames_);
  nlohmann_json_j.at("frames").get_to(nlohmann_json_t.frames_);
  if (nlohmann_json_j.contains("gaps")) nlohmann_json_j.at("gaps").get_to(nlohmann_json_t.gaps_);
  if (nlohmann_json_j.contains("truncated")) nlohmann_json_j.at("truncated").get_to(nlohmann_json_t.truncated_);
}

std::string to_string(capture_state in_state) {
  switch (in_state) {
    case capture_state::idle:
      return "idle";
    case capture_state::recording:
      return "recording";
    case capture_state::stopping:
      return "stopping";
    case capture_state::stopped:
      return "stopped";
    case capture_state::previewing:
      return "previewing";
  }
  return "unknown";
}

std::string to_string(error_code_t in_error_code) {
  switch (in_error_code) {
    case error_code_t::none:
      return "none";
    case error_code_t::already_recording:
      return "already_recording";
    case error_code_t::not_recording:
      return "not_recording";
    case error_code_t::not_streaming:
      return "not_streaming";
    case error_code_t::camera_open_failed:
      return "camera_open_failed";
    case error_code_t::camera_read_failed:
      return "camera_read_failed";
    case error_code_t::encode_failed:
      return "encode_failed";
    case error_code_t::raw_mkv_missing:
      return "raw_mkv_missing";
    case error_code_t::raw_frames_mismatch:
      return "raw_frames_mismatch";
    case error_code_t::png_sequence_invalid:
      return "png_sequence_invalid";
    case error_code_t::busy:
      return "busy";
    case error_code_t::unknown:
      return "unknown";
  }
  return "unknown";
}

FSys::path align_path(const FSys::path& in_session_dir) { return in_session_dir / "align.json"; }

void write_align(const align_record& in_record, const FSys::path& in_session_dir) {
  if (!FSys::exists(in_session_dir)) FSys::create_directories(in_session_dir);
  auto l_tmp_path = align_path(in_session_dir);
  l_tmp_path += ".tmp";

  {
    FSys::ofstream l_of{l_tmp_path, std::ios::out | std::ios::trunc};
    DOODLE_CHICK(l_of.is_open(), "无法写入 align.json: {}", l_tmp_path.generic_string());
    l_of << nlohmann::json{in_record}.dump(2);
  }

  std::error_code l_ec{};
  FSys::rename(l_tmp_path, align_path(in_session_dir), l_ec);
  if (l_ec) throw_exception(doodle_error{"重命名 align.json 失败: {}", l_ec.message()});
}

align_record read_align(const FSys::path& in_session_dir) {
  auto l_path = align_path(in_session_dir);
  DOODLE_CHICK(
      FSys::exists(l_path), "align.json 不存在: {} ({})", l_path.generic_string(),
      error_enum::file_not_exists
  );

  FSys::ifstream l_if{l_path, std::ios::in};
  DOODLE_CHICK(l_if.is_open(), "无法读取 align.json: {}", l_path.generic_string());
  return nlohmann::json::parse(l_if).get<align_record>();
}

std::optional<std::int64_t> wall_ms_of(const align_record& in_record, std::uint64_t in_seq) {
  for (const auto& l_frame : in_record.frames_) {
    if (l_frame.seq_ == in_seq) return l_frame.wall_ms_;
  }
  return std::nullopt;
}

std::optional<std::uint64_t> orig_seq_of(const align_record& in_record, std::uint64_t in_seq) {
  for (const auto& l_frame : in_record.frames_) {
    if (l_frame.seq_ == in_seq) return l_frame.orig_seq_;
  }
  return std::nullopt;
}

std::uint64_t expect_frame_count(const align_record& in_record) { return in_record.frames_.size(); }

}  // namespace doodle::face_capture
