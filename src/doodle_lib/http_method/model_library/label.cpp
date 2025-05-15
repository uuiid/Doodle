//
// Created by TD on 25-5-8.
//
#include "doodle_core/metadata/label.h"

#include "doodle_core/metadata/assets_file.h"
#include "doodle_core/sqlite_orm/sqlite_database.h"

#include "model_library.h"

namespace doodle::http::model_library {

boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_link_post::callback(
    http::session_data_ptr in_handle
) {
  auto l_uuid        = from_uuid_str(in_handle->capture_->get("id"));
  auto l_assets_uuid = from_uuid_str(in_handle->capture_->get("assets_id"));
  auto l_sql         = g_ctx().get<sqlite_database>();
  if (l_sql.has_assets_tree_assets_link(l_uuid, l_assets_uuid)) co_return in_handle->make_msg(nlohmann::json{});
  auto l_link             = std::make_shared<assets_file_helper::link_parent_t>();
  l_link->assets_type_uuid_  = l_uuid;
  l_link->assets_uuid_ = l_assets_uuid;
  co_await l_sql.install<assets_file_helper::link_parent_t>(l_link);
  co_return in_handle->make_msg(nlohmann::json{});
}
boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_link_delete_::callback(
    http::session_data_ptr in_handle
) {
  auto l_uuid        = from_uuid_str(in_handle->capture_->get("id"));
  auto l_assets_uuid = from_uuid_str(in_handle->capture_->get("assets_id"));
  auto l_sql         = g_ctx().get<sqlite_database>();
  if (!l_sql.has_assets_tree_assets_link(l_uuid, l_assets_uuid)) co_return in_handle->make_msg(nlohmann::json{});
  auto l_link = l_sql.get_assets_tree_assets_link(l_uuid, l_assets_uuid);
  co_await l_sql.remove<assets_file_helper::link_parent_t>(l_link.id_);
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::model_library