//
// Created by TD on 25-5-8.
//
#include "doodle_core/metadata/label.h"

#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include "model_library.h"

namespace doodle::http::model_library {

boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_link::post(http::session_data_ptr in_handle) {
  auto l_sql = g_ctx().get<sqlite_database>();
  if (l_sql.has_assets_tree_assets_link(id_, assets_id_)) co_return in_handle->make_msg(nlohmann::json{});
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 在 资产库节点 {} 中添加 资产库文件 {} ", person_.person_.email_,
      person_.person_.get_full_name(), id_, assets_id_
  );
  auto l_link               = std::make_shared<assets_file_helper::link_parent_t>();
  l_link->assets_type_uuid_ = id_;
  l_link->assets_uuid_      = assets_id_;
  co_await l_sql.install<assets_file_helper::link_parent_t>(l_link);
  co_return in_handle->make_msg(nlohmann::json{});
}
boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_link::delete_(
    http::session_data_ptr in_handle
) {
  auto l_sql = g_ctx().get<sqlite_database>();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 从 资产库节点 {} 移除 资产库文件 {} ", person_.person_.email_,
      person_.person_.get_full_name(), id_, assets_id_
  );
  if (!l_sql.has_assets_tree_assets_link(id_, assets_id_)) co_return in_handle->make_msg(nlohmann::json{});
  auto l_link = l_sql.get_assets_tree_assets_link(id_, assets_id_);
  co_await l_sql.remove<assets_file_helper::link_parent_t>(l_link.id_);
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::model_library