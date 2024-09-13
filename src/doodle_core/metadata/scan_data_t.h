//
// Created by TD on 24-9-11.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/metadata/project.h>

#include <entt/entt.hpp>
namespace doodle {

class scan_data_t {
 public:
  entt::handle handle_{};

  struct additional_data {
    FSys::path ue_path_;
    FSys::path rig_path_;
    FSys::path solve_path_;
    std::string name_;
    std::string version_;
    std::string num_;
  };

  struct database_t {
    uuid ue_uuid_;
    uuid rig_uuid_;
    uuid solve_uuid_;
    uuid project_;

    std::string ue_path_;
    std::string rig_path_;
    std::string solve_path_;
    std::string name_;
    std::string version_;
    std::string num_;

    std::int32_t id_{};
  };

  scan_data_t() = default;
  /// 获取函数
  explicit scan_data_t(const entt::handle& in_h) : handle_(in_h) {}
  explicit scan_data_t(entt::registry& in_registry) : handle_(in_registry, in_registry.create()) {
    in_registry.storage<entt::id_type>(detail::sql_id).emplace(handle_);
  }


  void ue_path(const FSys::path& in_path);

  void rig_path(const FSys::path& in_path);

  void solve_path(const FSys::path& in_path);

  void project(entt::entity in_project);

  void num_str(const std::string& in_num);

  void name(const std::string& in_name);

  void version(const std::string& in_version);

  void seed_to_sql();
  void destroy();
  static std::vector<entt::entity> load_from_sql(entt::registry& in_registry, const std::vector<database_t>& in_data);
};

}  // namespace doodle
