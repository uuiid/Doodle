//
// Created by TD on 25-1-16.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/base.h>

namespace doodle {

enum class entity_status {
  standby,
  running,
  complete,
  canceled,
};

struct DOODLE_CORE_API asset_instance_link {
  std::int32_t id_;
  uuid entity_id_;
  uuid asset_instance_id_;
};

struct DOODLE_CORE_API entity_link {
  std::int32_t id_;
  uuid entity_in_id_;
  uuid entity_out_id_;
  nlohmann::json data_;
  std::int32_t nb_occurences_;
  std::string label_;
};
struct DOODLE_CORE_API entity_concept_link {
  std::int32_t id_;
  uuid entity_id_;
  uuid entity_out_id_;
};
struct DOODLE_CORE_API entity {
  DOODLE_BASE_FIELDS();
  std::string name_;
  std::string code_;
  std::string description_;
  std::int32_t shotgun_id_;
  bool canceled_;

  std::optional<std::int32_t> nb_frames_;
  std::int32_t nb_entities_out_;
  bool is_casting_standby_;

  bool is_shared_;
  entity_status status_;

  // 外键
  /// 项目外键
  uuid project_id_;
  /// 实体类型外键
  uuid entity_type_id_;
  /// 父外键(未知)
  uuid parent_id_;
  /// 源外键(指向集数uuid)
  uuid source_id_;
  uuid preview_file_id_;
  nlohmann::json data_;

  uuid ready_for_;
  /// 创建人
  uuid created_by_;
  std::vector<uuid> entities_out;
  std::vector<uuid> entity_concept_links_;
  std::vector<uuid> instance_casting_;

  // to json
  friend void to_json(nlohmann::json& j, const entity& p) {
    j["id"]                   = p.uuid_id_;
    j["name"]                 = p.name_;
    j["code"]                 = p.code_;
    j["description"]          = p.description_;
    j["shotgun_id"]           = p.shotgun_id_;
    j["canceled"]             = p.canceled_;
    j["nb_frames"]            = p.nb_frames_;
    j["nb_entities_out"]      = p.nb_entities_out_;
    j["is_casting_standby"]   = p.is_casting_standby_;
    j["is_shared"]            = p.is_shared_;
    j["status"]               = p.status_;
    j["project_id"]           = p.project_id_;
    j["entity_type_id"]       = p.entity_type_id_;
    j["parent_id"]            = p.parent_id_;
    j["source_id"]            = p.source_id_;
    j["preview_file_id"]      = p.preview_file_id_;
    j["data"]                 = p.data_;
    j["ready_for"]            = p.ready_for_;
    j["created_by"]           = p.created_by_;
    j["entities_out"]         = p.entities_out;
    j["entity_concept_links"] = p.entity_concept_links_;
    j["instance_casting"]     = p.instance_casting_;
  }
};
}  // namespace doodle