//
// Created by TD on 24-10-15.
//

#include "doodle_core/exception/exception.h"
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_session_data.h>

#include "model_library.h"
#include <memory>
#include <treehh/tree.hh>
#include <vector>
namespace doodle::http::model_library {

void check_data(const assets_helper::database_t& in_data) {
  /// 检查是否存在引用自身
  if (!in_data.uuid_parent_.is_nil() && in_data.uuid_id_ == in_data.uuid_parent_)
    throw_exception(doodle_error{"{} 不能引用自身", in_data.uuid_id_});

  /// 检查是否存在循环引用
  const auto& l_list = g_ctx().get<sqlite_database>().get_all<assets_helper::database_t>();
  std::map<uuid, const assets_helper::database_t*> l_map{};
  for (const auto& l_item : l_list) {
    l_map[l_item.uuid_id_] = &l_item;
  }
  auto l_parent_uuid = in_data.uuid_parent_;
  for (int i = 0; i < 101; ++i) {
    if (l_parent_uuid.is_nil()) break;
    if (!l_map.contains(l_parent_uuid)) throw_exception(doodle_error{"{} 未找到父节点", in_data.uuid_id_});
    l_parent_uuid = l_map[l_parent_uuid]->uuid_parent_;
    if (i == 100) throw_exception(doodle_error{" {} 节点存在循环引用或者达到最大的深度", in_data.uuid_id_});
  }
}

boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_tree::get(
    http::session_data_ptr in_handle
) {
  person_.check_user();
  auto l_list = g_ctx().get<sqlite_database>().get_all<assets_helper::database_t>();

  nlohmann::json l_json{};
  l_json = l_list;
  co_return in_handle->make_msg(l_json);
}

boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_tree::post(
    http::session_data_ptr in_handle
) {
  person_.check_supervisor();
  auto l_json                                      = in_handle->get_json();
  std::shared_ptr<assets_helper::database_t> l_ptr = std::make_shared<assets_helper::database_t>(
      std::get<nlohmann::json>(in_handle->body_).get<assets_helper::database_t>()
  );

  if (!l_ptr->uuid_parent_.is_nil()) {
    if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_ptr->uuid_parent_);
        l_list == 0)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
  }
  co_await g_ctx().get<sqlite_database>().install<assets_helper::database_t>(l_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_tree::patch(
    http::session_data_ptr in_handle
) {
  person_.check_supervisor();
  auto l_values = std::make_shared<std::vector<assets_helper::database_t>>(
      in_handle->get_json().get<std::vector<assets_helper::database_t>>()
  );
  auto l_sql = g_ctx().get<sqlite_database>();
  for (auto&& l_value : *l_values) {
    check_data(l_value);
    l_value.id_ = l_sql.uuid_to_id<assets_helper::database_t>(l_value.uuid_id_);
  }
  co_await g_ctx().get<sqlite_database>().update_range<assets_helper::database_t>(l_values);
  co_return in_handle->make_msg(nlohmann::json{} = *l_values);
}

boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_tree_instance::put(
    http::session_data_ptr in_handle
) {
  person_.check_supervisor();
  auto l_value = std::make_shared<assets_helper::database_t>(
      g_ctx().get<sqlite_database>().get_by_uuid<assets_helper::database_t>(id_)
  );
  in_handle->get_json().get_to(*l_value);
  check_data(*l_value);
  co_await g_ctx().get<sqlite_database>().install<assets_helper::database_t>(l_value);
  co_return in_handle->make_msg(nlohmann::json{} = *l_value);
}

boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_tree_instance::delete_(
    http::session_data_ptr in_handle
) {
  person_.check_supervisor();
  auto l_uuid = boost::lexical_cast<uuid>(id_);
  auto l_sql  = g_ctx().get<sqlite_database>();
  if (l_sql.has_assets_tree_assets_link(l_uuid) || l_sql.has_assets_tree_child(l_uuid))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "该节点有子节点无法删除");
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 资产库节点 {} ", person_.person_.email_,
      person_.person_.get_full_name(), l_uuid
  );
  // else if (!l_r.empty()) {
  //   auto l_rem = std::make_shared<std::vector<std::int64_t>>();
  //   l_rem->reserve(l_r.size());
  //   for (const auto& l_item : l_r) {
  //     l_rem->push_back(l_item.id_);
  //   }
  //   co_await g_ctx().get<sqlite_database>().remove<assets_file_helper::database_t>(l_rem);
  // }
  co_await l_sql.remove<assets_helper::database_t>(l_uuid);

  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::model_library