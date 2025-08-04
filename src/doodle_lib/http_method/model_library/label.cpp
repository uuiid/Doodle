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
  if (!l_sql.has_assets_tree_assets_link(id_, assets_id_)) co_return in_handle->make_msg(nlohmann::json{});
  auto l_link = l_sql.get_assets_tree_assets_link(id_, assets_id_);
  co_await l_sql.remove<assets_file_helper::link_parent_t>(l_link.id_);
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::model_library