//
// Created by TD on 2021/5/7.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/metadata.h>

namespace doodle {

namespace assets_file_helper {
struct database_t {
  std::int32_t id_{};
  uuid uuid_id_{};
  std::string label_{};
  FSys::path path_{};
  std::string notes_{};
  // 激活
  bool active_{};
  uuid uuid_parent_{};
  std::int32_t parent_id_{};
  bool has_thumbnail_{};
  std::string extension_{};

  std::vector<uuid> labels_{};  // 多对多关系

  friend void DOODLE_CORE_API to_json(nlohmann::json& j, const database_t& p) {
    j["id"]            = p.uuid_id_;
    j["label"]         = p.label_;
    j["path"]          = p.path_;
    j["notes"]         = p.notes_;
    j["active"]        = p.active_;
    j["parent_id"]     = p.uuid_parent_;
    j["has_thumbnail"] = p.has_thumbnail_;
    j["extension"]     = p.extension_;
    j["labels"]        = p.labels_;
  }
  friend void DOODLE_CORE_API from_json(const nlohmann::json& j, database_t& p) {
    if (j.contains("label")) j.at("label").get_to(p.label_);
    if (j.contains("path")) j.at("path").get_to(p.path_);
    if (j.contains("notes")) j.at("notes").get_to(p.notes_);
    if (j.contains("active")) j.at("active").get_to(p.active_);
    if (j.contains("parent_id")) j.at("parent_id").get_to(p.uuid_parent_);
    if (j.contains("has_thumbnail")) j.at("has_thumbnail").get_to(p.has_thumbnail_);
    if (j.contains("extension"))
      j.at("extension").get_to(p.extension_);
    else
      p.extension_ = ".png";

    if (p.uuid_parent_.is_nil() || p.label_.empty())
      throw_exception(doodle_error{"必须的键值没有找到 uuid_parent_ {} label_ {}", p.uuid_parent_, p.label_});
  }
};
}  // namespace assets_file_helper
}  // namespace doodle