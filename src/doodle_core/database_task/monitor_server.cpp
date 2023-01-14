//
// Created by TD on 2023/1/14.
//

#include "monitor_server.h"

#include <doodle_core/core/core_sql.h>
#include <doodle_core/database_task/details/database.h>
#include <doodle_core/database_task/details/user.h>
#include <doodle_core/database_task/details/work_task.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_task.h>

#include <sqlpp11/sqlite3/sqlite3.h>
namespace doodle::database_n {

void monitor_server::monitor() {
  obs_create_database.connect(*reg, entt::collector.group<database>());
  obs_create_user.connect(*reg, entt::collector.group<database, user>());
  obs_create_work_task_info.connect(*reg, entt::collector.group<database, work_task_info>());

  obs_updata_database.connect(*reg, entt::collector.update<database>());
  obs_updata_user.connect(*reg, entt::collector.update<user>().where<database>());
  obs_updata_work_task_info.connect(*reg, entt::collector.update<work_task_info>().where<database>());

  reg->on_destroy<database>().connect<&monitor_server::database_on_destroy>(*this);
}

void monitor_server::database_on_destroy(entt::registry& in_reg, entt::entity in_entity) {
  auto&& l_d = in_reg.get<database>(in_entity);
  if (l_d.is_install()) database_on_destroy_data.emplace_back(l_d.get_id());
}

void monitor_server::load_all() {
  obs_create_database.clear();
  obs_create_user.clear();
  obs_create_work_task_info.clear();
  obs_updata_database.clear();
  obs_updata_user.clear();
  obs_updata_work_task_info.clear();
  database_on_destroy_data.clear();
  reg->clear();

  entt::locator<database_info>::value();
  {
    auto l_ptr_conn = database_info::value().get_connection_const();
    auto l_conn     = sqlpp::start_transaction(*l_ptr_conn);
    std::map<std::int64_t, entt::entity> l_id_map{};
    {
      sql_com<database, false> l_load1{reg};
      l_load1.select(l_ptr_conn, l_id_map);
    }
    {
      sql_com<user, false> l_load1{reg};
      l_load1.select(l_ptr_conn, l_id_map);
    }
    {
      sql_com<work_task_info, false> l_load1{reg};
      l_load1.select(l_ptr_conn, l_id_map);
    }
    l_conn.commit();
  }
  obs_create_database.clear();
  obs_create_user.clear();
  obs_create_work_task_info.clear();
  obs_updata_database.clear();
  obs_updata_user.clear();
  obs_updata_work_task_info.clear();
  database_on_destroy_data.clear();
}
}  // namespace doodle::database_n