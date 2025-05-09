//
// Created by TD on 25-5-8.
//
#include "doodle_core/metadata/label.h"

#include "doodle_core/sqlite_orm/sqlite_database.h"

#include "model_library.h"

namespace doodle::http::model_library {
boost::asio::awaitable<boost::beast::http::message_generator> label_get::callback(http::session_data_ptr in_handle) {
  nlohmann::json l_json{};
  l_json = g_ctx().get<sqlite_database>().get_all<label>();
  co_return in_handle->make_msg(l_json);
}

boost::asio::awaitable<boost::beast::http::message_generator> label_post::callback(http::session_data_ptr in_handle) {
  auto l_json  = in_handle->get_json();
  auto l_label = std::make_shared<label>();
  l_json.get_to(*l_label);
  l_label->uuid_id_ = core_set::get_set().get_uuid();
  co_await g_ctx().get<sqlite_database>().install(l_label);
  co_return in_handle->make_msg(nlohmann::json{} = *l_label);
}

boost::asio::awaitable<boost::beast::http::message_generator> label_put::callback(http::session_data_ptr in_handle) {
  const auto l_json      = in_handle->get_json();
  auto l_label           = std::make_shared<label>();
  const auto l_uuid      = from_uuid_str(in_handle->capture_->get("id"));
  auto l_sql             = g_ctx().get<sqlite_database>();
  const auto l_label_ptr = std::make_shared<label>(l_sql.get_by_uuid<label>(l_uuid));
  l_json.get_to(*l_label_ptr);

  co_await g_ctx().get<sqlite_database>().install(l_label_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_label_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> label_delete_::callback(
    http::session_data_ptr in_handle
) {
  auto l_uuid = from_uuid_str(in_handle->capture_->get("id"));
  auto l_sql  = g_ctx().get<sqlite_database>();
  if (l_sql.has_label_assets_link(l_uuid))
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "存在和标签关联的模型"});
  co_await l_sql.remove<label>(l_uuid);
  co_return in_handle->make_msg(nlohmann::json{});
}

boost::asio::awaitable<boost::beast::http::message_generator> label_link_post::callback(
    http::session_data_ptr in_handle
) {
  auto l_uuid        = from_uuid_str(in_handle->capture_->get("id"));
  auto l_assets_uuid = from_uuid_str(in_handle->capture_->get("assets_id"));
  auto l_sql         = g_ctx().get<sqlite_database>();
  if (l_sql.has_label_assets_link(l_uuid, l_assets_uuid)) co_return in_handle->make_msg(nlohmann::json{});
  auto l_link             = std::make_shared<label_assets_link>();
  l_link->label_uuid_id_  = l_uuid;
  l_link->assets_uuid_id_ = l_assets_uuid;
  co_await l_sql.install<label_assets_link>(l_link);
  co_return in_handle->make_msg(nlohmann::json{});
}
boost::asio::awaitable<boost::beast::http::message_generator> label_link_delete_::callback(
    http::session_data_ptr in_handle
) {
  auto l_uuid        = from_uuid_str(in_handle->capture_->get("id"));
  auto l_assets_uuid = from_uuid_str(in_handle->capture_->get("assets_id"));
  auto l_sql         = g_ctx().get<sqlite_database>();
  if (!l_sql.has_label_assets_link(l_uuid, l_assets_uuid)) co_return in_handle->make_msg(nlohmann::json{});
  auto l_link = l_sql.get_label_assets_link(l_uuid, l_assets_uuid);
  co_await l_sql.remove<label_assets_link>(l_link.id_);
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::model_library