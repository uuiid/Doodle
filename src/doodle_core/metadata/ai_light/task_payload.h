#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/ai_light/candidate.h>
#include <doodle_core/metadata/ai_light/common.h>
#include <doodle_core/metadata/ai_light/rig.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace doodle::ai_light {

constexpr std::string_view g_ai_light_render_report_file_name{"render_report.json"};

constexpr std::string_view g_ai_light_local_cache_flag{"ai_light"};

inline std::string make_iteration_relative_dir(const uuid& in_session_id, const uuid& in_iteration_id) {
  return fmt::format("{}/{}", in_session_id, in_iteration_id);
}

inline std::string make_candidate_file_name(const uuid& in_candidate_id, bool in_linear) {
  return fmt::format("{}{}", in_candidate_id, in_linear ? ".exr" : ".png");
}

inline std::string make_candidate_frames_dir(const uuid& in_candidate_id) { return fmt::format("{}/", in_candidate_id); }

inline std::string make_candidate_movie_file(const uuid& in_candidate_id) {
  return fmt::format("{}.mp4", in_candidate_id);
}

enum class ai_light_candidate_file_role {
  render,
  render_linear,
  render_movie,
};

struct DOODLE_CORE_API ai_light_assembly_file_t {
  FSys::path from_{};
  FSys::path to_{};

  friend void to_json(nlohmann::json& j, const ai_light_assembly_file_t& p) {
    j["from"] = p.from_;
    j["to"]   = p.to_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_assembly_file_t& p) {
    j.at("from").get_to(p.from_);
    j.at("to").get_to(p.to_);
  }
  bool operator==(const ai_light_assembly_file_t& in_other) const {
    return from_ == in_other.from_ && to_ == in_other.to_;
  }
};

struct DOODLE_CORE_API ai_light_task_candidate_t {
  uuid candidate_id_{};
  FSys::path frames_dir_{};
  FSys::path png_path_{};
  FSys::path exr_path_{};
  FSys::path movie_path_{};
  lighting_rig_t rig_{};

  friend void to_json(nlohmann::json& j, const ai_light_task_candidate_t& p) {
    j["candidate_id"]    = p.candidate_id_;
    j["frames_dir"]      = p.frames_dir_;
    j["png_path"]        = p.png_path_;
    j["exr_path"]        = p.exr_path_;
    j["movie_path"]      = p.movie_path_;
    j["rig"]             = p.rig_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_task_candidate_t& p) {
    if (j.contains("candidate_id")) j.at("candidate_id").get_to(p.candidate_id_);
    if (j.contains("frames_dir")) j.at("frames_dir").get_to(p.frames_dir_);
    if (j.contains("png_path")) j.at("png_path").get_to(p.png_path_);
    if (j.contains("exr_path")) j.at("exr_path").get_to(p.exr_path_);
    if (j.contains("movie_path")) j.at("movie_path").get_to(p.movie_path_);
    if (j.contains("rig")) j.at("rig").get_to(p.rig_);
  }
};

struct DOODLE_CORE_API ai_light_launch_plan_t {
  uuid session_id_{};
  uuid iteration_id_{};
  std::int32_t iteration_{0};
  FSys::path out_dir_{};
  std::string render_map_{};
  std::string level_sequence_{};
  std::string movie_pipeline_config_{};
  std::int32_t frame_begin_{0};
  std::int32_t frame_end_{0};
  std::string ue_project_path_{};
  std::string camera_file_path_{};
  FSys::path project_dir_{};
  std::vector<ai_light_assembly_file_t> assembly_files_{};
  std::vector<ai_light_task_candidate_t> candidates_{};

  friend void to_json(nlohmann::json& j, const ai_light_launch_plan_t& p) {
    j["session_id"]            = p.session_id_;
    j["iteration_id"]          = p.iteration_id_;
    j["iteration"]             = p.iteration_;
    j["out_dir"]               = p.out_dir_;
    j["render_map"]            = p.render_map_;
    j["level_sequence"]        = p.level_sequence_;
    j["movie_pipeline_config"] = p.movie_pipeline_config_;
    j["frame_begin"]           = p.frame_begin_;
    j["frame_end"]             = p.frame_end_;
    j["ue_project_path"]       = p.ue_project_path_;
    j["camera_file_path"]      = p.camera_file_path_;
    j["project_dir"]           = p.project_dir_;
    j["assembly_files"]        = p.assembly_files_;
    j["candidates"]            = p.candidates_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_launch_plan_t& p) {
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("iteration_id")) j.at("iteration_id").get_to(p.iteration_id_);
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("out_dir")) j.at("out_dir").get_to(p.out_dir_);
    if (j.contains("render_map")) j.at("render_map").get_to(p.render_map_);
    if (j.contains("level_sequence")) j.at("level_sequence").get_to(p.level_sequence_);
    if (j.contains("movie_pipeline_config")) j.at("movie_pipeline_config").get_to(p.movie_pipeline_config_);
    if (j.contains("frame_begin")) j.at("frame_begin").get_to(p.frame_begin_);
    if (j.contains("frame_end")) j.at("frame_end").get_to(p.frame_end_);
    if (j.contains("ue_project_path")) j.at("ue_project_path").get_to(p.ue_project_path_);
    if (j.contains("camera_file_path")) j.at("camera_file_path").get_to(p.camera_file_path_);
    j.at("project_dir").get_to(p.project_dir_);
    j.at("assembly_files").get_to(p.assembly_files_);
    if (j.contains("candidates")) j.at("candidates").get_to(p.candidates_);
  }
};

struct DOODLE_CORE_API ai_light_task_payload_t {
  uuid session_id_{};
  std::int32_t iteration_{0};
  FSys::path out_dir_{};
  std::string baseline_level_{};
  std::string render_map_{};
  std::string level_sequence_{};
  std::string movie_pipeline_config_{};
  std::int32_t frame_begin_{0};
  std::int32_t frame_end_{0};
  std::string ue_project_path_{};
  std::string camera_file_path_{};
  FSys::path project_dir_{};
  std::vector<ai_light_assembly_file_t> assembly_files_{};
  std::vector<ai_light_task_candidate_t> candidates_{};

  friend void to_json(nlohmann::json& j, const ai_light_task_payload_t& p) {
    j["session_id"]            = p.session_id_;
    j["iteration"]             = p.iteration_;
    j["out_dir"]               = p.out_dir_;
    j["baseline_level"]        = p.baseline_level_;
    j["render_map"]            = p.render_map_;
    j["level_sequence"]        = p.level_sequence_;
    j["movie_pipeline_config"] = p.movie_pipeline_config_;
    j["frame_begin"]           = p.frame_begin_;
    j["frame_end"]             = p.frame_end_;
    j["ue_project_path"]       = p.ue_project_path_;
    j["camera_file_path"]      = p.camera_file_path_;
    j["project_dir"]           = p.project_dir_;
    j["assembly_files"]        = p.assembly_files_;
    j["candidates"]            = p.candidates_;
  }
  friend void from_json(const nlohmann::json& j, ai_light_task_payload_t& p) {
    if (j.contains("session_id")) j.at("session_id").get_to(p.session_id_);
    if (j.contains("iteration")) j.at("iteration").get_to(p.iteration_);
    if (j.contains("out_dir")) j.at("out_dir").get_to(p.out_dir_);
    if (j.contains("baseline_level")) j.at("baseline_level").get_to(p.baseline_level_);
    if (j.contains("render_map")) j.at("render_map").get_to(p.render_map_);
    if (j.contains("level_sequence")) j.at("level_sequence").get_to(p.level_sequence_);
    if (j.contains("movie_pipeline_config")) j.at("movie_pipeline_config").get_to(p.movie_pipeline_config_);
    if (j.contains("frame_begin")) j.at("frame_begin").get_to(p.frame_begin_);
    if (j.contains("frame_end")) j.at("frame_end").get_to(p.frame_end_);
    if (j.contains("ue_project_path")) j.at("ue_project_path").get_to(p.ue_project_path_);
    if (j.contains("camera_file_path")) j.at("camera_file_path").get_to(p.camera_file_path_);
    j.at("project_dir").get_to(p.project_dir_);
    j.at("assembly_files").get_to(p.assembly_files_);
    if (j.contains("candidates")) j.at("candidates").get_to(p.candidates_);
  }
};

struct DOODLE_CORE_API candidate_render_report_t {
  uuid candidate_id_{};
  candidate_render_status status_{candidate_render_status::rendered};
  std::int64_t render_ms_{0};
  std::int32_t light_count_{0};
  std::int32_t frame_count_{0};
  candidate_metrics_t metrics_{};

  friend void to_json(nlohmann::json& j, const candidate_render_report_t& p) {
    j["candidate_id"]    = p.candidate_id_;
    j["status"]          = p.status_;
    j["render_ms"]       = p.render_ms_;
    j["light_count"]     = p.light_count_;
    j["frame_count"]     = p.frame_count_;
    j["metrics"]         = p.metrics_;
  }
  friend void from_json(const nlohmann::json& j, candidate_render_report_t& p) {
    j.at("candidate_id").get_to(p.candidate_id_);
    j.at("status").get_to(p.status_);
    if (j.contains("render_ms")) j.at("render_ms").get_to(p.render_ms_);
    if (j.contains("light_count")) j.at("light_count").get_to(p.light_count_);
    if (j.contains("frame_count")) j.at("frame_count").get_to(p.frame_count_);
    if (j.contains("metrics")) j.at("metrics").get_to(p.metrics_);
  }
};

struct DOODLE_CORE_API round_render_report_t {
  std::vector<candidate_render_report_t> candidates_{};
  std::int32_t frame_rate_{0};
  std::string error_{};

  friend void to_json(nlohmann::json& j, const round_render_report_t& p) {
    j["candidates"] = p.candidates_;
    j["frame_rate"] = p.frame_rate_;
    j["error"]      = p.error_;
  }
  friend void from_json(const nlohmann::json& j, round_render_report_t& p) {
    j.at("candidates").get_to(p.candidates_);
    if (j.contains("frame_rate")) j.at("frame_rate").get_to(p.frame_rate_);
    if (j.contains("error")) j.at("error").get_to(p.error_);
  }
};

}  // namespace doodle::ai_light
