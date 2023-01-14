//
// Created by TD on 2023/1/14.
//

#include "monitor_server.h"

#include <doodle_core/core/core_sql.h>
#include <doodle_core/database_task/details/database.h>
#include <doodle_core/database_task/details/user.h>
#include <doodle_core/database_task/details/work_task.h>
#include <doodle_core/logger/logger.h>
#include <doodle_core/metadata/metadata.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/metadata/work_task.h>

#include <sqlpp11/sqlite3/sqlite3.h>
namespace doodle::database_n {

void monitor_server::monitor() {
  obs_create_database.connect(*reg, entt::collector.group<database>());
  obs_create_user.connect(*reg, entt::collector.group<database, user>());
  obs_create_work_task_info.connect(*reg, entt::collector.group<database, work_task_info>());

  obs_updata_user.connect(*reg, entt::collector.update<user>().where<database>());
  obs_updata_work_task_info.connect(*reg, entt::collector.update<work_task_info>().where<database>());

  reg->on_destroy<database>().connect<&monitor_server::database_on_destroy>(*this);

  run_tick();
}

void monitor_server::database_on_destroy(entt::registry& in_reg, entt::entity in_entity) {
  auto&& l_d = in_reg.get<database>(in_entity);
  if (l_d.is_install()) database_on_destroy_data.emplace_back(l_d.get_id());
}

void monitor_server::load_all() {
  save();
  timer.cancel();
  reg->clear();

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
  obs_updata_user.clear();
  obs_updata_work_task_info.clear();
  database_on_destroy_data.clear();

  run_tick();
}

void monitor_server::tick(const boost::system::error_code& in_code) {
  if (in_code == boost::asio::error::operation_aborted) {
    DOODLE_LOG_INFO(in_code.message());
    return;
  }

  if (obs_create_database.size() > 10 || obs_create_user.size() > 10 || obs_create_work_task_info.size() > 10 ||
      obs_updata_user.size() > 10 || obs_updata_work_task_info.size() > 10 || database_on_destroy_data.size() > 10)
    save();
  run_tick();
}

void monitor_server::run_tick() {
  timer.expires_after(100ms);
  timer.async_wait([this](const boost::system::error_code& in_code) { this->tick(in_code); });
}
void monitor_server::save() {
  try {
    auto l_ptr_conn = database_info::value().get_connection_const();
    auto l_conn     = sqlpp::start_transaction(*l_ptr_conn);
    std::map<std::int64_t, entt::entity> l_id_map{};
    {
      sql_com<database, false> l_load1{reg};
      l_load1.insert(l_ptr_conn, obs_create_database);
      l_load1.destroy(l_ptr_conn, database_on_destroy_data);
    }
    {
      sql_com<user, false> l_load1{reg};
      l_load1.insert(l_ptr_conn, obs_create_user);
      l_load1.update(l_ptr_conn, obs_updata_user);
      l_load1.destroy(l_ptr_conn, database_on_destroy_data);
    }
    {
      sql_com<work_task_info, false> l_load1{reg};
      l_load1.insert(l_ptr_conn, obs_create_work_task_info);
      l_load1.update(l_ptr_conn, obs_updata_work_task_info);
      l_load1.destroy(l_ptr_conn, database_on_destroy_data);
    }
    l_conn.commit();
  } catch (const sqlpp::exception& in_error) {
    DOODLE_LOG_INFO(boost::diagnostic_information(in_error));
  }

  obs_create_database.clear();
  obs_create_user.clear();
  obs_create_work_task_info.clear();
  obs_updata_user.clear();
  obs_updata_work_task_info.clear();
  database_on_destroy_data.clear();
}
}  // namespace doodle::database_n