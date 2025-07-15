//
// Created by TD on 25-3-11.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
// 状态
enum class preview_file_statuses { processing, ready, broken };
NLOHMANN_JSON_SERIALIZE_ENUM(
    preview_file_statuses,
    {
        {preview_file_statuses::processing, "processing"},
        {preview_file_statuses::ready, "ready"},
        {preview_file_statuses::broken, "broken"},
    }
)
// 验证 状态
enum class preview_file_validation_statuses { validated, rejected, neutral };
NLOHMANN_JSON_SERIALIZE_ENUM(
    preview_file_validation_statuses,
    {
        {preview_file_validation_statuses::validated, "validated"},
        {preview_file_validation_statuses::rejected, "rejected"},
        {preview_file_validation_statuses::neutral, "neutral"},
    }
)
struct preview_file {
  DOODLE_BASE_FIELDS()

  std::string name_;
  std::string original_name_;
  std::int32_t revision_;
  /// 预览图在评论中的位置, 可能是多个预览图, 需要区分位置
  std::int32_t position_;
  std::string extension_;
  std::string description_;
  FSys::path path_;
  std::string source_;
  std::int64_t file_size_;
  preview_file_statuses status_;
  preview_file_validation_statuses validation_status_;
  nlohmann::json annotations_;
  std::int32_t width_;
  std::int32_t height_;
  std::int32_t duration_;

  std::int32_t shotgun_id_;
  bool is_movie_;
  std::string url_;
  std::string uploaded_movie_url_;
  std::string uploaded_movie_name_;

  chrono::system_zoned_time created_at_;
  chrono::system_zoned_time updated_at_;

  // 外键
  uuid task_id_;
  uuid person_id_;
  uuid source_file_id_;
  // to json
  friend void to_json(nlohmann::json& j, const preview_file& p) {
    j["id"]                  = p.uuid_id_;
    j["name"]                = p.name_;
    j["original_name"]       = p.original_name_;
    j["revision"]            = p.revision_;
    j["position"]            = p.position_;
    j["extension"]           = p.extension_;
    j["description"]         = p.description_;
    j["path"]                = p.path_;
    j["source"]              = p.source_;
    j["file_size"]           = p.file_size_;
    j["status"]              = p.status_;
    j["validation_status"]   = p.validation_status_;
    j["annotations"]         = p.annotations_;
    j["width"]               = p.width_;
    j["height"]              = p.height_;
    j["duration"]            = p.duration_;
    j["shotgun_id"]          = p.shotgun_id_;
    j["is_movie"]            = p.is_movie_;
    j["url"]                 = p.url_;
    j["uploaded_movie_url"]  = p.uploaded_movie_url_;
    j["uploaded_movie_name"] = p.uploaded_movie_name_;

    j["created_at"]          = p.created_at_;
    j["updated_at"]          = p.updated_at_;

    j["task_id"]             = p.task_id_;
    j["person_id"]           = p.person_id_;
    j["source_file_id"]      = p.source_file_id_;
  }
};
}  // namespace doodle
