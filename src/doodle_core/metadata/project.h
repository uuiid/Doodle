#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
namespace doodle {

namespace project_helper {
struct database_t;
}

namespace project_helper {

struct database_t {
  std::int32_t id_{};
  uuid uuid_id_{};

  std::string name_{};
  std::filesystem::path path_{};
  std::string en_str_{};
  std::string shor_str_{};
  std::string auto_upload_path_{};

  friend void to_json(nlohmann::json& j, const database_t& p) {
    j["name"] = p.name_;
    j["path"] = p.path_;
  }
  friend void from_json(const nlohmann::json& j, database_t& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("path")) j.at("path").get_to(p.path_);
    if (j.contains("en_str")) j.at("en_str").get_to(p.en_str_);
    if (j.contains("code")) j.at("code").get_to(p.shor_str_);
    if (j.contains("auto_upload_path")) j.at("auto_upload_path").get_to(p.auto_upload_path_);
  }
};

};  // namespace project_helper

}  // namespace doodle
