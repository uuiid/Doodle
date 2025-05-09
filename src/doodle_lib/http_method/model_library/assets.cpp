//
// Created by TD on 24-10-15.
//

#include "doodle_core/metadata/label.h"
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include "boost/lexical_cast.hpp"

#include "model_library.h"
#include <jwt-cpp/jwt.h>
namespace doodle::http::model_library {

boost::asio::awaitable<boost::beast::http::message_generator> assets_get::callback(http::session_data_ptr in_handle) {
  auto l_list = g_ctx().get<sqlite_database>().get_all<assets_file_helper::database_t>();
  nlohmann::json l_json{};
  l_json = l_list;
  co_return in_handle->make_msg(l_json);
}
boost::asio::awaitable<boost::beast::http::message_generator> assets_post::callback(http::session_data_ptr in_handle) {
  auto l_json = in_handle->get_json();
  if (l_json.is_object()) {
    std::shared_ptr<assets_file_helper::database_t> l_ptr = std::make_shared<assets_file_helper::database_t>();
    l_json.get_to(*l_ptr);
    // 额外添加标签
    l_json["labels"].get_to(l_ptr->labels_);

    if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_ptr->uuid_parent_);
        l_list == 0)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
    else
      l_ptr->parent_id_ = l_list;

    l_ptr->uuid_id_ = core_set::get_set().get_uuid();
    co_await g_ctx().get<sqlite_database>().install<assets_file_helper::database_t>(l_ptr);
    auto l_labels_link = std::make_shared<std::vector<label_assets_link>>();
    for (auto&& i : l_ptr->labels_)
      l_labels_link->emplace_back(label_assets_link{.label_uuid_id_ = i, .assets_uuid_id_ = l_ptr->uuid_id_});
    co_await g_ctx().get<sqlite_database>().install_range<label_assets_link>(l_labels_link);

    co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
  }
  if (l_json.is_array()) {
    std::shared_ptr<std::vector<assets_file_helper::database_t>> l_ptr =
        std::make_shared<std::vector<assets_file_helper::database_t>>();
    *l_ptr = l_json.get<std::vector<assets_file_helper::database_t>>();

    for (auto& i : *l_ptr) {
      if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(i.uuid_parent_);
          l_list == 0)
        co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
      else
        i.parent_id_ = l_list;

      i.uuid_id_ = core_set::get_set().get_uuid();
    }
    co_await g_ctx().get<sqlite_database>().install_range<assets_file_helper::database_t>(l_ptr);
    co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
  }
  co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求格式");
}
boost::asio::awaitable<boost::beast::http::message_generator> assets_modify_post::callback(
    http::session_data_ptr in_handle
) {
  uuid l_uuid = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));

  std::shared_ptr<assets_file_helper::database_t> const l_ptr = std::make_shared<assets_file_helper::database_t>(
      g_ctx().get<sqlite_database>().get_by_uuid<assets_file_helper::database_t>(l_uuid)
  );
  in_handle->get_json().get_to(*l_ptr);

  if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_ptr->uuid_parent_);
      l_list == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
  else
    l_ptr->parent_id_ = l_list;

  if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_file_helper::database_t>(l_uuid); l_list == 0)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到节点");
  else
    l_ptr->id_ = l_list;

  l_ptr->uuid_id_ = l_uuid;
  co_await g_ctx().get<sqlite_database>().install<assets_file_helper::database_t>(l_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> assets_patch::callback(http::session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");

  auto& l_json = std::get<nlohmann::json>(in_handle->body_);
  std::shared_ptr<std::vector<assets_file_helper::database_t>> l_ptr =
      std::make_shared<std::vector<assets_file_helper::database_t>>();
  std::vector<uuid> l_uuid_list{};
  *l_ptr = l_json.get<std::vector<assets_file_helper::database_t>>();
  for (auto&& l_obj : l_json) {
    l_uuid_list.emplace_back(l_obj["id"].get<uuid>());
  }
  for (int i = 0; i < l_ptr->size(); ++i) (*l_ptr)[i].uuid_id_ = l_uuid_list[i];

  for (auto& i : *l_ptr) {
    if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(i.uuid_parent_); l_list == 0)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
    else
      i.parent_id_ = l_list;
    if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_file_helper::database_t>(i.uuid_id_);
        l_list == 0)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到节点");
    else
      i.id_ = l_list;
  }

  co_await g_ctx().get<sqlite_database>().install_range<assets_file_helper::database_t>(l_ptr);

  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> assets_delete_::callback(
    http::session_data_ptr in_handle
) {
  auto l_uuid = boost::lexical_cast<uuid>(in_handle->capture_->get("id"));
  auto l_p = get_person(in_handle);
  l_p->is_manager();

  co_await g_ctx().get<sqlite_database>().remove<assets_file_helper::database_t>(l_uuid);
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::model_library