#pragma once
#include <doodle_core/core/core_set.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>
#include <doodle_core/metadata/metadata_descriptor.h>
#include <doodle_core/metadata/person.h>

#include <cmath>


namespace doodle {

enum class project_styles {
  e2d,
  e2dpaper,
  e3d,
  e2d3d,
  ar,
  vfx,
  stop,
  motion,
  archviz,
  commercial,
  catalog,
  immersive,
  nft,
  video,
  vr,
};

NLOHMANN_JSON_SERIALIZE_ENUM(
    project_styles, {{project_styles::e2d, "2D Animation"},
                     {project_styles::e2dpaper, "2D Animation (Paper)"},
                     {project_styles::e3d, "3D Animation"},
                     {project_styles::e2d3d, "2D/3D Animation"},
                     {project_styles::ar, "Augmented Reality"},
                     {project_styles::vfx, "VFX"},
                     {project_styles::stop, "Stop Motion"},
                     {project_styles::motion, "Motion Design"},
                     {project_styles::archviz, "Commercial"},
                     {project_styles::commercial, "commercial"},
                     {project_styles::catalog, "Catalog"},
                     {project_styles::immersive, "Immersive Experience"},
                     {project_styles::nft, "NFT Collection"},
                     {project_styles::video, "Video Game"},
                     {project_styles::vr, "Virtual Reality"}}
);

struct project_person_link {
  std::int64_t id_;
  uuid project_id_;
  uuid person_id_;
  std::optional<std::int32_t> shotgun_id_;
};
struct project_task_type_link {
  DOODLE_BASE_FIELDS();
  uuid project_id_;
  uuid task_type_id_;
  std::optional<std::int32_t> priority_;

  // form json
  friend void from_json(const nlohmann::json& j, project_task_type_link& p) {
    j.at("task_type_id").get_to(p.task_type_id_);
    if (j.contains("project_id")) j.at("project_id").get_to(p.project_id_);
    if (j.contains("priority")) j.at("priority").get_to(p.priority_);
  }
  // to json
  friend void to_json(nlohmann::json& j, const project_task_type_link& p) {
    j["task_type_id"] = p.task_type_id_;
    j["project_id"]   = p.project_id_;
    j["priority"]     = p.priority_;
  }
};

struct project_task_status_link {
  DOODLE_BASE_FIELDS();

  uuid project_id_;
  uuid task_status_id_;
  std::optional<std::int32_t> priority_;
  std::vector<person_role_type> roles_for_board_{
      person_role_type::user,    person_role_type::admin,  person_role_type::supervisor,
      person_role_type::manager, person_role_type::client, person_role_type::vendor,
  };

  // form json
  friend void from_json(const nlohmann::json& j, project_task_status_link& p) {
    j.at("project_id").get_to(p.project_id_);
    j.at("task_status_id").get_to(p.task_status_id_);
    if (j.contains("priority")) j.at("priority").get_to(p.priority_);
    if (j.contains("roles_for_board")) j.at("roles_for_board").get_to(p.roles_for_board_);
  }
  // to json
  friend void to_json(nlohmann::json& j, const project_task_status_link& p) {
    j["project_id"]      = p.project_id_;
    j["task_status_id"]  = p.task_status_id_;
    j["priority"]        = p.priority_;
    j["roles_for_board"] = p.roles_for_board_;
  }
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
  std::int32_t fps_;
  std::string ratio_;
  std::string resolution_;
  std::string production_type_;
  project_styles production_style_;
  std::optional<chrono::system_zoned_time> start_date_;
  std::optional<chrono::system_zoned_time> end_date_;
  std::optional<std::int32_t> man_days_;
  std::int32_t nb_episodes_;
  std::int32_t episode_span_;
  std::int32_t max_retakes_;
  bool is_clients_isolated_;
  bool is_preview_download_allowed_;
  bool is_set_preview_automated_;
  std::string homepage_{"assets"};
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

  /// 我们自己的数据
  FSys::path path_{};                  // 项目路径
  std::string en_str_{};               // 项目拼音名称
  std::string auto_upload_path_{};     // 项目自动上传路径
  std::string production_category_{};  // 电影所属类别
  std::string short_name_{};           // 显示的简称
  FSys::path asset_root_path_{};       // 资产的根路径(相对于项目路径)
  // std::pair<width,height>
  std::pair<std::int32_t, std::int32_t> get_resolution() const;
  std::double_t get_film_aperture() const;
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
    if (j.contains("path")) j.at("path").get_to(p.path_);
    if (j.contains("en_str")) j.at("en_str").get_to(p.en_str_);
    if (j.contains("auto_upload_path")) j.at("auto_upload_path").get_to(p.auto_upload_path_);
    if (j.contains("production_category")) j.at("production_category").get_to(p.production_category_);
    if (j.contains("short_name")) j.at("short_name").get_to(p.short_name_);
    if (j.contains("asset_root_path")) j.at("asset_root_path").get_to(p.asset_root_path_);
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

    j["path"]                               = p.path_;
    j["en_str"]                             = p.en_str_;
    j["auto_upload_path"]                   = p.auto_upload_path_;
    j["production_category"]                = p.production_category_;
    j["short_name"]                         = p.short_name_;
    j["asset_root_path"]                    = p.asset_root_path_;
  }
};

struct project_minimal {
  project_minimal() = default;
  project_minimal(const project& in_project)
      : uuid_id_(in_project.uuid_id_),
        name_(in_project.name_),
        path_(in_project.path_),
        en_str_(in_project.en_str_),
        auto_upload_path_(in_project.auto_upload_path_),
        code_(in_project.code_) {}

  uuid uuid_id_{};

  std::string name_{};
  FSys::path path_{};
  std::string en_str_{};
  std::string auto_upload_path_{};
  std::string code_{};

  friend void to_json(nlohmann::json& j, const project_minimal& p) {
    j["id"]               = p.uuid_id_;
    j["name"]             = p.name_;
    j["path"]             = p.path_;
    j["en_str"]           = p.en_str_;
    j["auto_upload_path"] = p.auto_upload_path_;
    j["code"]             = p.code_;
  }

  friend void from_json(const nlohmann::json& j, project_minimal& p) {
    j.at("id").get_to(p.uuid_id_);
    j.at("name").get_to(p.name_);
    j.at("path").get_to(p.path_);
    j.at("en_str").get_to(p.en_str_);
    j.at("auto_upload_path").get_to(p.auto_upload_path_);
    j.at("code").get_to(p.code_);
  }
};

struct project_with_extra_data : project {
  std::vector<metadata_descriptor> descriptors_;
  std::vector<project_task_type_link> task_types_priority_;
  std::vector<project_task_status_link> task_statuses_link_;

  // to json
  friend void to_json(nlohmann::json& j, const project_with_extra_data& p) {
    to_json(j, static_cast<const project&>(p));

    j["descriptors"] = nlohmann::json::value_t::array;
    for (const auto& d : p.descriptors_) {
      j["descriptors"].push_back(
          nlohmann::json{
              {"id", d.uuid_id_},
              {"name", d.name_},
              {"field_name", d.field_name_},
              {"data_type", d.data_type_},
              {"choices", d.choices_},
              {"for_client", d.for_client_},
              {"entity_type", d.entity_type_},
              {"departments", d.department_},
          }
      );
    }

    for (const auto& d : p.task_types_priority_) {
      j["task_types_priority"][fmt::to_string(d.task_type_id_)] = d.priority_;
    }

    for (const auto& d : p.task_statuses_link_) {
      j["task_statuses_link"][fmt::to_string(d.task_status_id_)] =
          nlohmann::json{{"priority", d.priority_}, {"roles_for_board", d.roles_for_board_}};
    }
  }
};

}  // namespace doodle
