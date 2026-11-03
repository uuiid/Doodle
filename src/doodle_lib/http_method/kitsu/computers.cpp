#include "computers.h"

#include "doodle_core/metadata/computer.h"

#include "doodle_lib/core/app_base.h"
#include "doodle_lib/core/http/http_function.h"
#include "doodle_lib_fwd.h"
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>

#include <boost/asio/awaitable.hpp>
#include <boost/asio/buffers_iterator.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/consign.hpp>
#include <boost/asio/post.hpp>
#include <boost/beast/websocket/rfc6455.hpp>
#include <boost/beast/websocket/stream.hpp>
#include <boost/lockfree/detail/uses_optional.hpp>
#include <boost/lockfree/spsc_value.hpp>
#include <boost/scope/scope_exit.hpp>

#include "core/global_function.h"
#include <atomic>
#include <chrono>
#include <fmt/ranges.h>
#include <functional>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include <memory>
#include <set>
#include <spdlog/spdlog.h>
#include <string>

namespace doodle::http {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers, get) {
  person_.check_not_outsourcer();
  auto l_sql       = get_sqlite_database();
  auto l_computers = l_sql.get_all<computer>();
  auto l_now = std::chrono::system_clock::now();
  for (auto& i : l_computers) 
    if (i.last_heartbeat_time_.get_sys_time() + std::chrono::seconds(30) < l_now) 
      i.status_ = computer_status::offline;
  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = l_computers);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers, post) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  auto l_computer                 = l_json.get<computer>();
  l_computer.last_heartbeat_time_ = chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()};

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  if (auto l_db_computer = l_sql.get_computer_by_hardware_id(l_computer.hardware_id_); l_db_computer.has_value()) {
    l_computer.uuid_id_ = l_db_computer->uuid_id_;
    l_sqls.emplace_back(update(l_sql)
                            .from<computer>()
                            .set(c(&computer::status_) = l_computer.status_)
                            .set(c(&computer::last_heartbeat_time_) = l_computer.last_heartbeat_time_)
                            .where(c(&computer::uuid_id_) == l_computer.uuid_id_));
  } else {
    l_sqls.emplace_back(insert(l_sql).into<computer>().values(l_computer));
  }
  co_await l_sql.run_sql(std::move(l_sqls));
  socket_io::broadcast(socket_io::computer_update_broadcast_t{.computer_id_ = l_computer.uuid_id_});
  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = l_computer);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers_instance, get) {
  person_.check_not_outsourcer();
  auto l_sql      = get_sqlite_database();
  auto l_computer = l_sql.get_by_uuid<computer>(computer_id_);
  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = l_computer);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers_instance, put) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  auto l_update = update(l_sql)
                      .from<computer>()
                      .set_from_ref<computer>(l_json)
                      .set(
                          c(&computer::last_heartbeat_time_) =
                              chrono::system_zoned_time{chrono::current_zone(), chrono::system_clock::now()}
                      )
                      .where(c(&computer::uuid_id_) == computer_id_);
  l_sqls.emplace_back(std::move(l_update));
  co_await l_sql.run_sql(std::move(l_sqls));

  auto l_computer = l_sql.get_by_uuid<computer>(computer_id_);
  socket_io::broadcast(socket_io::computer_update_broadcast_t{.computer_id_ = computer_id_});
  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = l_computer);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_computers_instance, delete_) {
  person_.check_not_outsourcer();
  auto l_sql      = get_sqlite_database();
  auto l_computer = l_sql.get_by_uuid<computer>(computer_id_);
  using namespace orm;
  sql_modify_statement_vector_t l_sqls{};
  l_sqls.emplace_back(delete_from(l_sql).from<computer>().where(c(&computer::uuid_id_) == computer_id_));
  co_await l_sql.run_sql(std::move(l_sqls));
  socket_io::broadcast(socket_io::computer_delete_broadcast_t{.computer_id_ = computer_id_});
  co_return std::make_unique<http_body_json_t>(nlohmann::json{} = l_computer);
}


}  // namespace doodle::http
