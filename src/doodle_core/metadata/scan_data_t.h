//
// Created by TD on 24-9-11.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/project.h>

#include <entt/entt.hpp>
namespace doodle {

class scan_data_t {
  static void dependent_uuid(entt::registry& in_reg, entt::entity in_entity);
  static void on_destroy(entt::registry& in_reg, entt::entity in_entity);

 public:
  struct scan_data_ctx_t {
    std::array<entt::scoped_connection, 3> conn_;
  };

  struct path_uuid {
    uuid ue_uuid_;
    uuid rig_uuid_;
    uuid solve_uuid_;
  };

  struct additional_data {
    FSys::path ue_path_;
    FSys::path rig_path_;
    FSys::path solve_path_;
    std::string name_;
    std::string version_;
    std::string num_;
  };

  struct database_t {
    uuid uuid_id_;

    std::optional<uuid> ue_uuid_;
    std::optional<uuid> rig_uuid_;
    std::optional<uuid> solve_uuid_;
    std::int32_t project_id_;

    std::optional<std::filesystem::path> ue_path_;
    std::optional<std::filesystem::path> rig_path_;
    std::optional<std::filesystem::path> solve_path_;
    std::int32_t season_;
    details::assets_type_enum dep_{};

    std::string name_;
    std::optional<std::string> version_;
    std::optional<std::string> num_;

    std::string hash_;

    std::int32_t id_;
  };

  scan_data_t() = default;
  /// 获取函数

  static void seed_to_sql(const entt::registry& in_registry, const std::vector<entt::entity>& in_entity);
  static std::vector<entt::entity> load_from_sql(entt::registry& in_registry, const std::vector<database_t>& in_data);
};

}  // namespace doodle
