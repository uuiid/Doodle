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
  uuid object_id_; // 关联对象ID 实体ID
  std::string object_type_{"Task"};
  std::string text_;
  nlohmann::json data_;
  nlohmann::json replies_{nlohmann::json::array()};
  nlohmann::json checklist_{nlohmann::json::array()};
  bool pinned_;
  std::vector<std::string> links;
  chrono::system_zoned_time created_at_{chrono::current_zone(), chrono::system_clock::now()};
  chrono::system_zoned_time updated_at_{chrono::current_zone(), chrono::system_clock::now()};

  void set_comment_mentions(const uuid& in_project_id);
  void set_comment_department_mentions();

  // 外键
  uuid task_status_id_;
  uuid person_id_;
  uuid editor_id_;  // 编辑人
  uuid preview_file_id_;
  std::vector<uuid> previews_;
  std::vector<uuid> mentions_;             // 评论提到的人
  std::vector<uuid> department_mentions_;  // 评论提到的部门
  std::vector<uuid> acknowledgements_;
  std::vector<uuid> attachment_files_;
  // to json
  friend void to_json(nlohmann::json& j, const comment& p) {
    j["id"]                  = p.uuid_id_;
    j["shotgun_id"]          = p.shotgun_id_;
    j["object_id"]           = p.object_id_;
    j["object_type"]         = p.object_type_;
    j["text"]                = p.text_;
    j["data"]                = p.data_;
    j["replies"]             = p.replies_;
    j["checklist"]           = p.checklist_;
    j["pinned"]              = p.pinned_;
    j["links"]               = p.links;
    j["created_at"]          = p.created_at_;
    j["updated_at"]          = p.updated_at_;
    j["task_status_id"]      = p.task_status_id_;
    j["person_id"]           = p.person_id_;
    j["editor_id"]           = p.editor_id_;
    j["preview_file_id"]     = p.preview_file_id_;
    j["previews"]            = p.previews_;
    j["mentions"]            = p.mentions_;
    j["department_mentions"] = p.department_mentions_;
    j["acknowledgements"]    = p.acknowledgements_;
    j["attachment_files"]    = p.attachment_files_;
  }

  // form json
  friend void from_json(const nlohmann::json& j, comment& p) {
    if (j.contains("checklist")) j.at("checklist").get_to(p.checklist_);
    if (j.contains("comment")) {
      if (auto&& l_c = j.at("comment"); l_c.is_string())
        l_c.get_to(p.text_);
      else if (l_c.is_boolean())
        p.text_ = l_c.get<bool>() ? "true" : "false";
      else if (l_c.is_number())
        p.text_ = fmt::to_string(l_c.get<std::double_t>());
    }
    if (j.contains("task_status_id")) j.at("task_status_id").get_to(p.task_status_id_);
    if (j.contains("object_id")) j.at("object_id").get_to(p.object_id_);
    if (j.contains("text")) j.at("text").get_to(p.text_);
  }
};

}  // namespace doodle
