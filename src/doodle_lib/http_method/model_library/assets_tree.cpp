//
// Created by TD on 24-10-15.
//
#include "assets_tree.h"

#include <doodle_core/metadata/assets.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_session_data.h>
namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_get(session_data_ptr in_handle) {
  auto l_list = g_ctx().get<sqlite_database>().get_all<assets_helper::database_t>();

  std::map<std::int64_t, assets_helper::database_t*> l_map{};
  for (auto&& l : l_list) l_map[l.id_] = &l;

  for (auto&& l : l_list)
    if (l.parent_id_) l.uuid_parent_ = l_map[*l.parent_id_]->uuid_id_;

  nlohmann::json l_json{};
  l_json = l_list;
  co_return in_handle->make_msg(l_json.dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_post(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  std::shared_ptr<assets_helper::database_t> l_ptr = std::make_shared<assets_helper::database_t>();
  try {
    *l_ptr = std::get<nlohmann::json>(in_handle->body_).get<assets_helper::database_t>();
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
    );
  }
  if (!l_ptr->uuid_parent_.is_nil()) {
    if (auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<assets_helper::database_t>(l_ptr->uuid_parent_);
        l_list.empty())
      co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
    else
      l_ptr->parent_id_ = l_list.front().id_;
  }
  l_ptr->uuid_id_ = core_set::get_set().get_uuid();
  if (auto l_r = co_await g_ctx().get<sqlite_database>().install<assets_helper::database_t>(l_ptr); !l_r)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());

  co_return in_handle->make_msg(nlohmann::json{*l_ptr}.dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_post_modify(session_data_ptr in_handle) {
  uuid l_uuid{};

  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");

  auto l_value = std::make_shared<assets_helper::database_t>();
  try {
    l_uuid = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));
    *l_value = std::get<nlohmann::json>(in_handle->body_).get<assets_helper::database_t>();
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id 或者 无效的数据");
  }
  if (auto l_r = g_ctx().get<sqlite_database>().get_by_uuid<assets_helper::database_t>(l_uuid); l_r.empty())
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "无效的id, 未能再库中查找到实体");
  else
    l_value->id_ = l_r.front().id_;

  if (auto l_r = co_await g_ctx().get<sqlite_database>().install<assets_helper::database_t>(l_value); !l_r)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());

  co_return in_handle->make_msg(nlohmann::json{*l_value}.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_delete(session_data_ptr in_handle) {
  auto l_uuid = std::make_shared<uuid>();
  try {
    *l_uuid = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }

  if (auto l_r = co_await g_ctx().get<sqlite_database>().remove<assets_helper::database_t>(l_uuid); !l_r)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());

  co_return in_handle->make_msg("");
}
}  // namespace
void assets_tree_reg(http_route& in_http_route) {
  in_http_route
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/model_library/assets_tree", assets_tree_get
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::post, "api/doodle/model_library/assets_tree", assets_tree_post
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::post, "api/doodle/model_library/assets_tree/{id}", assets_tree_post_modify
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::delete_, "api/doodle/model_library/assets_tree/{id}", assets_tree_delete
      )

      );
}
}  // namespace doodle::http::kitsu