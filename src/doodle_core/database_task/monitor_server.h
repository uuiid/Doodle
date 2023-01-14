//
// Created by TD on 2023/1/14.
//

#pragma once
#include <doodle_core/doodle_core_fwd.h>

#include <utility>
namespace doodle::database_n {

class monitor_server {
  registry_ptr reg{};

  entt::observer obs_create_database{};
  entt::observer obs_create_user{};
  entt::observer obs_create_work_task_info{};

  entt::observer obs_updata_database{};
  entt::observer obs_updata_user{};
  entt::observer obs_updata_work_task_info{};

  std::vector<std::int64_t> database_on_destroy_data{};
  void database_on_destroy(entt::registry& in_reg, entt::entity);

 public:
  monitor_server() : monitor_server(g_reg()){};
  explicit monitor_server(registry_ptr in_registry) : reg(std::move(in_registry)) { monitor(); };

  void monitor();

  void load_all();
};

}  // namespace doodle::database_n
