//
// Created by TD on 2026-11-6.
//

#pragma once

#include <doodle_core/lib_warp/json_warp.h>

#include <doodle_lib/doodle_lib_fwd.h>

#include <boost/uuid/uuid.hpp>

#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>

namespace doodle::face_capture {

constexpr std::int32_t k_fps{25};

constexpr std::int64_t k_frame_interval_ms{1000 / k_fps};

constexpr std::int32_t k_keyint{k_fps};

constexpr std::int32_t k_png_digits{6};

constexpr std::int32_t k_silent_timeout_ms{1000};

constexpr std::int32_t k_max_raw_queue_frames{300};

constexpr std::int32_t k_stop_drain_timeout_ms{2000};

constexpr std::int32_t k_max_capture_queue_frames{120};

constexpr std::int32_t k_default_max_frames{999'999};

constexpr std::int32_t k_default_width{1280};

constexpr std::int32_t k_default_height{720};

enum class capture_state : std::int32_t {
  idle       = 0,
  recording  = 1,
  stopping   = 2,
  stopped    = 3,
  previewing = 4,
};
DOODLE_JSON_SERIALIZE_ENUM_STRICT(
    capture_state, {
                       {capture_state::idle, "idle"},
                       {capture_state::recording, "recording"},
                       {capture_state::stopping, "stopping"},
                       {capture_state::stopped, "stopped"},
                       {capture_state::previewing, "previewing"},
                   }
)
struct camera_info {
  std::int32_t index_{0};
  std::string name_{};
  std::string backend_{};
  std::int32_t width_{0};
  std::int32_t height_{0};

  friend void to_json(nlohmann::json& nlohmann_json_j, const camera_info& nlohmann_json_t);
  friend void from_json(const nlohmann::json& nlohmann_json_j, camera_info& nlohmann_json_t);
};

struct capture_option {
  std::int32_t device_index_{0};
  std::int32_t width_{k_default_width};
  std::int32_t height_{k_default_height};

  friend void to_json(nlohmann::json& nlohmann_json_j, const capture_option& nlohmann_json_t);
  friend void from_json(const nlohmann::json& nlohmann_json_j, capture_option& nlohmann_json_t);
};

struct capture_status {
  capture_state state_{capture_state::idle};
  std::int32_t width_{0};
  std::int32_t height_{0};
  std::int32_t fps_{k_fps};
  std::uint64_t frames_{0};
  std::uint64_t raw_frames_{0};
  std::uint64_t inserted_frames_{0};
  double source_fps_{0.0};
  std::int64_t duration_ms_{0};
  bool truncated_{false};

  friend void to_json(nlohmann::json& nlohmann_json_j, const capture_status& nlohmann_json_t);
};

struct gap_range {
  std::int64_t wall_ms_begin_{0};
  std::int64_t wall_ms_end_{0};
  std::int64_t duration_ms_{0};

  friend void to_json(nlohmann::json& nlohmann_json_j, const gap_range& nlohmann_json_t);
  friend void from_json(const nlohmann::json& nlohmann_json_j, gap_range& nlohmann_json_t);
};

struct align_frame {
  std::uint64_t seq_{0};
  std::uint64_t orig_seq_{0};
  std::int64_t wall_ms_{0};

  friend void to_json(nlohmann::json& nlohmann_json_j, const align_frame& nlohmann_json_t);
  friend void from_json(const nlohmann::json& nlohmann_json_j, align_frame& nlohmann_json_t);
};

struct align_record {
  boost::uuids::uuid session_id_{};
  std::int32_t width_{0};
  std::int32_t height_{0};
  std::int32_t fps_{k_fps};
  double source_fps_{0.0};
  std::string started_at_{};
  std::int64_t duration_ms_{0};
  std::uint64_t raw_frames_{0};
  std::uint64_t inserted_frames_{0};
  std::vector<align_frame> frames_{};
  std::vector<gap_range> gaps_{};
  bool truncated_{false};

  friend void to_json(nlohmann::json& nlohmann_json_j, const align_record& nlohmann_json_t);
  friend void from_json(const nlohmann::json& nlohmann_json_j, align_record& nlohmann_json_t);
};

struct fragment_t {
  std::vector<std::uint8_t> data_{};
  std::uint64_t seq_begin_{0};
  std::uint64_t seq_end_{0};
  bool key_{false};
};

}  // namespace doodle::face_capture
