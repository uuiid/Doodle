#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
#include <doodle_core/metadata/person.h>
namespace doodle {

namespace project_helper {
struct database_t;
}

struct project_person_link {
  std::int64_t id_;
  uuid project_id_;
  uuid person_id_;
};
struct project_task_type_link {
  DOODLE_BASE_FIELDS();
  uuid project_id_;
  uuid task_type_id_;
  std::int32_t priority_;
};

struct project_task_status_link {
  DOODLE_BASE_FIELDS();

  uuid project_id_;
  uuid task_status_id_;
  std::int32_t priority_;
  std::vector<person_role_type> roles_for_board_;
};
struct project_asset_type_link {
  std::int64_t id_;
  uuid project_id_;
  uuid asset_type_id_;
};

struct project_status_automation_link {
  std::int64_t id_;
  uuid project_id_;
  uuid status_automation_id_;
};

struct project_preview_background_file_link {
  std::int64_t id_;
  uuid project_id_;
  uuid preview_background_file_id_;
};

struct project {
  DOODLE_BASE_FIELDS();

  std::string name_;
  std::string code_;
  std::string description_;
  std::int64_t shotgun_id_;
  nlohmann::json file_tree_;
  nlohmann::json data_;
  bool has_avatar_;
  std::string fps_;
  std::string ratio_;
  std::string resolution_;
  std::string production_type_;
  std::string production_style_;
  chrono::system_zoned_time start_date_;
  chrono::system_zoned_time end_date_;
  std::int32_t man_days_;
  std::int32_t nb_episodes_;
  std::int32_t episode_span_;
  std::int32_t max_retakes_;
  bool is_clients_isolated_;
  bool is_preview_download_allowed_;
  bool is_set_preview_automated_;
  bool homepage_;
  bool is_publish_default_for_artists_;
  std::int32_t hd_bitrate_compression_;
  std::int32_t ld_bitrate_compression_;
  uuid project_status_id_;
  uuid default_preview_background_file_id_;
  std::vector<uuid> team_;
  std::vector<uuid> asset_types_;
  std::vector<uuid> task_statuses_;
  std::vector<uuid> task_types_;
  std::vector<uuid> status_automations_;
  std::vector<uuid> preview_background_files_;
};
namespace project_helper {

struct database_t : boost::equality_comparable<database_t> {
  std::int32_t id_{};
  uuid uuid_id_{};

  std::string name_{};
  std::filesystem::path path_{};
  std::string en_str_{};
  std::string auto_upload_path_{};
  std::string code_{};

  bool operator==(const database_t& p) const {
    return std::tie(uuid_id_, name_, id_, path_, en_str_, auto_upload_path_, code_) ==
           std::tie(p.uuid_id_, p.name_, p.id_, p.path_, p.en_str_, p.auto_upload_path_, p.code_);
  }

  friend void to_json(nlohmann::json& j, const database_t& p) {
    j["name"]             = p.name_;
    j["id"]               = p.id_;
    j["path"]             = p.path_;
    j["en_str"]           = p.en_str_;
    j["auto_upload_path"] = p.auto_upload_path_;
    j["code"]             = p.code_;
  }
  friend void from_json(const nlohmann::json& j, database_t& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("path")) j.at("path").get_to(p.path_);
    if (j.contains("en_str")) j.at("en_str").get_to(p.en_str_);
    if (j.contains("auto_upload_path")) j.at("auto_upload_path").get_to(p.auto_upload_path_);
    if (j.contains("code")) j.at("code").get_to(p.code_);
  }
};

};  // namespace project_helper

}  // namespace doodle
