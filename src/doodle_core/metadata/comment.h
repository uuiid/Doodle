//
// Created by TD on 2021/5/18.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
namespace doodle {
struct comment_preview_link {
  std::int32_t id_;
  uuid comment_id_;
  uuid preview_file_id_;
};
struct comment_mentions {
  std::int32_t id_;
  uuid comment_id_;
  uuid person_id_;
};
struct comment_department_mentions {
  std::int32_t id_;
  uuid comment_id_;
  uuid department_id_;
};
struct comment_acknoledgments {
  std::int32_t id_;
  uuid comment_id_;
  uuid person_id_;
};
struct DOODLE_CORE_API comment {
  DOODLE_BASE_FIELDS();
  std::int32_t shotgun_id_;
  uuid object_id_;
  std::string object_type_;
  std::string text_;
  nlohmann::json data_;
  nlohmann::json replies_;
  nlohmann::json checklist_;
  bool pinned_;
  std::vector<std::string> links;

  // 外键
  uuid task_status_id_;
  uuid person_id_;
  uuid editor_id_;
  uuid preview_file_id_;
  std::vector<uuid> previews_;
  std::vector<uuid> mentions_;
  std::vector<uuid> department_mentions_;
  std::vector<uuid> acknowledgements_;
  std::vector<uuid> attachment_files_;
};

}  // namespace doodle
