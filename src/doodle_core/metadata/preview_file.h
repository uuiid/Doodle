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
struct preview_file {
  DOODLE_BASE_FIELDS()

  std::string name_;
  std::string original_name_;
  std::int32_t revision_;
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
};
}  // namespace doodle
