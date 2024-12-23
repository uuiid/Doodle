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

#include <tl/expected.hpp>
#include <treehh/tree.hh>
namespace doodle::http::kitsu {
namespace {

boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_get(session_data_ptr in_handle) {
  auto l_list = g_ctx().get<sqlite_database>().get_all<assets_helper::database_t>();

  nlohmann::json l_json{};
  l_json = l_list;
  co_return in_handle->make_msg(l_json.dump());
}

tl::expected<void, std::string> check_data(const assets_helper::database_t& in_data) {
  /// 检查是否存在引用自身
  if (!in_data.uuid_parent_.is_nil() && in_data.uuid_id_ == in_data.uuid_parent_)
    return tl::make_unexpected(fmt::format(" {} 不能引用自身", in_data.uuid_id_));

  /// 检查是否存在循环引用
  const auto& l_list = g_ctx().get<sqlite_database>().get_all<assets_helper::database_t>();
  std::map<uuid, const assets_helper::database_t*> l_map{};
  for (const auto& l_item : l_list) {
    l_map[l_item.uuid_id_] = &l_item;
  }
  auto l_parent_uuid = in_data.uuid_parent_;
  for (int i = 0; i < 101; ++i) {
    if (l_parent_uuid.is_nil()) break;
    if (!l_map.contains(l_parent_uuid)) return tl::make_unexpected(fmt::format("{} 未找到父节点", in_data.uuid_id_));
    l_parent_uuid = l_map[l_parent_uuid]->uuid_parent_;
    if (i == 100) return tl::make_unexpected(fmt::format(" {} 节点存在循环引用或者达到最大的深度", in_data.uuid_id_));
  }
  return {};
}

boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_post(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  auto& l_json = std::get<nlohmann::json>(in_handle->body_);
  if (l_json.is_object()) {
    std::shared_ptr<assets_helper::database_t> l_ptr = std::make_shared<assets_helper::database_t>();
    try {
      *l_ptr = std::get<nlohmann::json>(in_handle->body_).get<assets_helper::database_t>();
    } catch (...) {
      co_return in_handle->make_error_code_msg(
          boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
      );
    }
    if (!l_ptr->uuid_parent_.is_nil()) {
      if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_ptr->uuid_parent_);
          l_list == 0)
        co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
    }
    l_ptr->uuid_id_ = core_set::get_set().get_uuid();
    co_await g_ctx().get<sqlite_database>().install<assets_helper::database_t>(l_ptr);
    co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
  }
  if (l_json.is_array()) {
    std::shared_ptr<std::vector<assets_helper::database_t>> l_ptr =
        std::make_shared<std::vector<assets_helper::database_t>>();
    try {
      *l_ptr = l_json.get<std::vector<assets_helper::database_t>>();
    } catch (...) {
      co_return in_handle->make_error_code_msg(
          boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
      );
    }
    for (auto& l_item : *l_ptr) {
      if (!l_item.uuid_parent_.is_nil()) {
        if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_item.uuid_parent_);
            l_list == 0)
          co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
      }
      l_item.uuid_id_ = core_set::get_set().get_uuid();
    }
    co_await g_ctx().get<sqlite_database>().install_range<assets_helper::database_t>(l_ptr);
    co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
  }
  co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的数据");
}

boost::asio::awaitable<boost::beast::http::message_generator> assets_tree_patch(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  auto& l_json = std::get<nlohmann::json>(in_handle->body_);
  {
    std::shared_ptr<std::vector<assets_helper::database_t>> l_ptr =
        std::make_shared<std::vector<assets_helper::database_t>>();
    try {
      *l_ptr = l_json.get<std::vector<assets_helper::database_t>>();
      for (int i = 0; i < l_ptr->size(); ++i) {
        (*l_ptr)[i].uuid_id_ = l_json[i]["id"].get<uuid>();
      }
    } catch (...) {
      co_return in_handle->make_error_code_msg(
          boost::beast::http::status::internal_server_error, boost::current_exception_diagnostic_information()
      );
    }
    for (auto& l_item : *l_ptr) {
      if (auto l_r = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_item.uuid_id_); l_r == 0)
        co_return in_handle->make_error_code_msg(
            boost::beast::http::status::internal_server_error, "无效的id, 未能再库中查找到实体"
        );
      else
        l_item.id_ = l_r;

      if (auto l_c = check_data(l_item); !l_c)
        co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, l_c.error());
    }
    co_await g_ctx().get<sqlite_database>().install_range<assets_helper::database_t>(l_ptr);
    co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
  }
  co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的数据");
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
  if (auto l_c = check_data(*l_value); !l_c)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, l_c.error());

  co_await g_ctx().get<sqlite_database>().install<assets_helper::database_t>(l_value);

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
    if (const auto l_r = g_ctx().get<sqlite_database>().get_by_parent_id<assets_helper::database_t>(l_uuid_);
        !l_r.empty())
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "该节点有子节点无法删除");

    if (auto l_r = g_ctx().get<sqlite_database>().get_by_parent_id<assets_file_helper::database_t>(l_uuid_);
        !l_r.empty() &&
        std::ranges::any_of(l_r, [](const assets_file_helper::database_t& l_item) -> bool { return l_item.active_; }))
      co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "该节点有子节点无法删除");
    else if (!l_r.empty()) {
      auto l_rem = std::make_shared<std::vector<std::int64_t>>();
      l_rem->reserve(l_r.size());
      for (const auto& l_item : l_r) {
        l_rem->push_back(l_item.id_);
      }
      co_await g_ctx().get<sqlite_database>().remove<assets_file_helper::database_t>(l_rem);
    }
  }
  co_await g_ctx().get<sqlite_database>().remove<assets_helper::database_t>(l_uuid);

  co_return in_handle->make_msg("{}");
}
}  // namespace
void assets_tree_reg(http_route& in_http_route) {
  in_http_route
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/doodle/model_library/assets_tree", assets_tree_get
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/model_library/assets_tree", assets_tree_post
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post, "api/doodle/model_library/assets_tree/{id}", assets_tree_post_modify
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::delete_, "api/doodle/model_library/assets_tree/{id}", assets_tree_delete
          )

      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::patch, "api/doodle/model_library/assets_tree", assets_tree_patch
      )
      )


  ;
}
}  // namespace doodle::http::kitsu