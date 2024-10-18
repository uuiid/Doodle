//
// Created by TD on 24-10-15.
//
#include "assets_tree.h"

#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_session_data.h>

#include <treehh/tree.hh>
namespace doodle::http::kitsu {
namespace {

boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_get(session_data_ptr in_handle) {
  auto l_list = g_ctx().get<sqlite_database>().get_all<assets_helper::database_t>();

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
  if (l_ptr->uuid_parent_) {
    if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(*l_ptr->uuid_parent_);
        l_list == 0)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
  }
  l_ptr->uuid_id_ = core_set::get_set().get_uuid();
  if (auto l_r = co_await g_ctx().get<sqlite_database>().install<assets_helper::database_t>(l_ptr); !l_r)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());

  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_post_modify(session_data_ptr in_handle) {
  uuid l_uuid{};

  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");

  auto l_value = std::make_shared<assets_helper::database_t>();
  try {
    l_uuid   = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));
    *l_value = std::get<nlohmann::json>(in_handle->body_).get<assets_helper::database_t>();
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id 或者 无效的数据");
  }
  if (auto l_r = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_uuid); l_r == 0)
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::internal_server_error, "无效的id, 未能再库中查找到实体"
    );
  else {
    l_value->id_      = l_r;
    l_value->uuid_id_ = l_uuid;
  }

  {
    /// 检查是否存在循环引用
    const auto& l_list = g_ctx().get<sqlite_database>().get_all<assets_helper::database_t>();
    std::map<uuid, const assets_helper::database_t*> l_map{};
    for (const auto& l_item : l_list) {
      l_map[l_item.uuid_id_] = &l_item;
    }
    auto l_parent_uuid = l_value->uuid_parent_.value_or(uuid{});
    for (int i = 0; i < 101; ++i) {
      if (l_parent_uuid.is_nil()) break;
      if (!l_map.contains(l_parent_uuid))
        co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
      l_parent_uuid = l_map[l_parent_uuid]->uuid_parent_.value_or(uuid{});
      if (i == 100)
        co_return in_handle->make_error_code_msg(
            boost::beast::http::status::not_found, "节点存在循环引用或者达到最大的深度"
        );
    }
  }

  if (auto l_r = co_await g_ctx().get<sqlite_database>().install<assets_helper::database_t>(l_value); !l_r)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());

  co_return in_handle->make_msg((nlohmann::json{} = *l_value).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_delete(session_data_ptr in_handle) {
  auto l_uuid = std::make_shared<uuid>();
  try {
    *l_uuid = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的任务id");
  }

  if (auto l_e = g_ctx().get<sqlite_database>().get_by_uuid<assets_helper::database_t>(*l_uuid); !l_e.empty()) {
    auto& l_uuid_ = l_e.front().uuid_id_;
    if (auto l_r = g_ctx().get<sqlite_database>().get_by_parent_id<assets_helper::database_t>(l_uuid_); !l_r.empty())
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "该节点有子节点无法删除");

    if (auto l_r = g_ctx().get<sqlite_database>().get_by_parent_id<assets_file_helper::database_t>(l_uuid_);
        !l_r.empty())
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "该节点有子节点无法删除");
  }

  if (auto l_r = co_await g_ctx().get<sqlite_database>().remove<assets_helper::database_t>(l_uuid); !l_r)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, l_r.error());

  co_return in_handle->make_msg("{}");
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