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
  /// 源外键(未知)
  uuid source_id_;
  uuid preview_file_id_;
  nlohmann::json data_;

  uuid ready_for_;
  /// 创建人
  uuid created_by_;
  std::vector<uuid> entities_out;
  std::vector<uuid> entity_concept_links_;
  std::vector<uuid> instance_casting_;
};
}  // namespace doodle