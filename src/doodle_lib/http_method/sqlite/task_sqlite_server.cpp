#include "task_sqlite_server.h"

#include <doodle_core/metadata/server_task_info.h>

#include <boost/asio.hpp>
namespace doodle::http {

void task_sqlite_server::init(const sql_connection_ptr& in_conn) {}

void task_sqlite_server::on_destroy(entt::registry& in_reg, entt::entity in_entt) {}

void task_sqlite_server::connect(entt::registry& in_registry_ptr) {
  obs_update_.connect(in_registry_ptr, entt::collector.update<server_task_info>());
  obs_create_.connect(in_registry_ptr, entt::collector.group<server_task_info>());
  conn_ = in_registry_ptr.on_destroy<server_task_info>().connect<&task_sqlite_server::on_destroy>(*this);
}
void task_sqlite_server::disconnect() {
  obs_update_.disconnect();
  obs_create_.disconnect();
  if (conn_) conn_.release();
}

void task_sqlite_server::clear() {
  obs_update_.clear();
  obs_create_.clear();
  destroy_ids_.clear();
}

void task_sqlite_server::run() {
  timer_ptr_  = std::make_shared<timer_t>(g_io_context());
  logger_ptr_ = g_logger_ctrl().make_log("task_sqlite_server");
  connect(*g_reg());
  begin_save();
}
void task_sqlite_server::begin_save() {
  timer_ptr_->expires_after(std::chrono::seconds(1));
  timer_ptr_->async_wait([this](const boost::system::error_code& in_ec) {
    if (in_ec) {
      logger_ptr_->log(log_loc(), level::warn, "begin_save error:{}", in_ec.message());
      return;
    }
    save();
    begin_save();
  });
}

std::tuple<std::vector<entt::entity>, std::vector<boost::uuids::uuid>> task_sqlite_server::get_data() const {
  std::vector<entt::entity> l_data{};
  for (const auto& entity : obs_update_) {
    l_data.push_back(entity);
  }
  for (const auto& entity : obs_create_) {
    l_data.push_back(entity);
  }
  l_data |= ranges::actions::unique;
  auto l_destroy_ids = destroy_ids_;
  l_destroy_ids |= ranges::actions::unique;
  return {std::move(l_data), l_destroy_ids};
}

void task_sqlite_server::save() {
  auto&& [l_save, l_del] = get_data();
  clear();
  if (l_save.empty() && l_del.empty()) return;
  std::vector<server_task_info> l_save_list{};
  std::ranges::transform(l_save, std::back_inserter(l_save_list), [](auto in_entt) {
    return g_reg()->get<server_task_info>(in_entt);
  });
}

}  // namespace doodle::http