#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
#include <doodle_core/metadata/metadata_descriptor.h>
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
  std::optional<std::int32_t> priority_;
};

struct project_task_status_link {
  DOODLE_BASE_FIELDS();

  uuid project_id_;
  uuid task_status_id_;
  std::optional<std::int32_t> priority_;
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
  std::optional<std::int64_t> shotgun_id_;
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
  std::optional<bool> is_publish_default_for_artists_;
  std::optional<std::int32_t> hd_bitrate_compression_;
  std::optional<std::int32_t> ld_bitrate_compression_;
  uuid project_status_id_;
  uuid default_preview_background_file_id_;
  std::vector<uuid> team_;
  std::vector<uuid> asset_types_;
  std::vector<uuid> task_statuses_;
  std::vector<uuid> task_types_;
  std::vector<uuid> status_automations_;
  std::vector<uuid> preview_background_files_;
  friend void from_json(const nlohmann::json& j, project& p) {
    if (j.contains("name")) j.at("name").get_to(p.name_);
    if (j.contains("code")) j.at("code").get_to(p.code_);
    if (j.contains("description")) j.at("description").get_to(p.description_);
    if (j.contains("shotgun_id")) j.at("shotgun_id").get_to(p.shotgun_id_);
    if (j.contains("file_tree")) j.at("file_tree").get_to(p.file_tree_);
    if (j.contains("data")) j.at("data").get_to(p.data_);
    if (j.contains("has_avatar")) j.at("has_avatar").get_to(p.has_avatar_);
    if (j.contains("fps")) j.at("fps").get_to(p.fps_);
    if (j.contains("ratio")) j.at("ratio").get_to(p.ratio_);
    if (j.contains("resolution")) j.at("resolution").get_to(p.resolution_);
    if (j.contains("production_type")) j.at("production_type").get_to(p.production_type_);
    if (j.contains("production_style")) j.at("production_style").get_to(p.production_style_);
    if (j.contains("start_date")) j.at("start_date").get_to(p.start_date_);
    if (j.contains("end_date")) j.at("end_date").get_to(p.end_date_);
    if (j.contains("man_days")) j.at("man_days").get_to(p.man_days_);
    if (j.contains("nb_episodes")) j.at("nb_episodes").get_to(p.nb_episodes_);
    if (j.contains("episode_span")) j.at("episode_span").get_to(p.episode_span_);
    if (j.contains("max_retakes")) j.at("max_retakes").get_to(p.max_retakes_);
    if (j.contains("is_clients_isolated")) j.at("is_clients_isolated").get_to(p.is_clients_isolated_);
    if (j.contains("is_preview_download_allowed"))
      j.at("is_preview_download_allowed").get_to(p.is_preview_download_allowed_);
    if (j.contains("is_set_preview_automated")) j.at("is_set_preview_automated").get_to(p.is_set_preview_automated_);
    if (j.contains("homepage")) j.at("homepage").get_to(p.homepage_);
    if (j.contains("is_publish_default_for_artists"))
      j.at("is_publish_default_for_artists").get_to(p.is_publish_default_for_artists_);
    if (j.contains("hd_bitrate_compression")) j.at("hd_bitrate_compression").get_to(p.hd_bitrate_compression_);
    if (j.contains("ld_bitrate_compression")) j.at("ld_bitrate_compression").get_to(p.ld_bitrate_compression_);
    if (j.contains("project_status_id")) j.at("project_status_id").get_to(p.project_status_id_);
    if (j.contains("default_preview_background_file_id"))
      j.at("default_preview_background_file_id").get_to(p.default_preview_background_file_id_);
  }
  friend void to_json(nlohmann::json& j, const project& p) {
    j["id"]                                 = p.uuid_id_;
    j["name"]                               = p.name_;
    j["code"]                               = p.code_;
    j["description"]                        = p.description_;
    j["shotgun_id"]                         = p.shotgun_id_;
    j["file_tree"]                          = p.file_tree_;
    j["data"]                               = p.data_;
    j["has_avatar"]                         = p.has_avatar_;
    j["fps"]                                = p.fps_;
    j["ratio"]                              = p.ratio_;
    j["resolution"]                         = p.resolution_;
    j["production_type"]                    = p.production_type_;
    j["production_style"]                   = p.production_style_;
    j["start_date"]                         = p.start_date_;
    j["end_date"]                           = p.end_date_;
    j["man_days"]                           = p.man_days_;
    j["nb_episodes"]                        = p.nb_episodes_;
    j["episode_span"]                       = p.episode_span_;
    j["max_retakes"]                        = p.max_retakes_;
    j["is_clients_isolated"]                = p.is_clients_isolated_;
    j["is_preview_download_allowed"]        = p.is_preview_download_allowed_;
    j["is_set_preview_automated"]           = p.is_set_preview_automated_;
    j["homepage"]                           = p.homepage_;
    j["is_publish_default_for_artists"]     = p.is_publish_default_for_artists_;
    j["hd_bitrate_compression"]             = p.hd_bitrate_compression_;
    j["ld_bitrate_compression"]             = p.ld_bitrate_compression_;
    j["project_status_id"]                  = p.project_status_id_;
    j["default_preview_background_file_id"] = p.default_preview_background_file_id_;

    j["team"]                               = p.team_;
    j["asset_types"]                        = p.asset_types_;
    j["task_statuses"]                      = p.task_statuses_;
    j["task_types"]                         = p.task_types_;
    j["status_automations"]                 = p.status_automations_;
    j["preview_background_files"]           = p.preview_background_files_;
  }
};

struct project_with_extra_data : project {
  std::vector<metadata_descriptor> descriptors_;
  std::vector<project_task_type_link> task_types_priority_;
  std::vector<project_task_status_link> task_statuses_link_;

  // to json
  friend void to_json(nlohmann::json& j, const project_with_extra_data& p) {
    to_json(j, static_cast<const project&>(p));
  }
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
