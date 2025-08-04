//
// Created by TD on 24-10-15.
//

#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/http/http_session_data.h>

#include "model_library.h"
#include <tl/expected.hpp>
#include <treehh/tree.hh>
namespace doodle::http::model_library {

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

boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_tree::get(
    http::session_data_ptr in_handle
) {
  auto l_list = g_ctx().get<sqlite_database>().get_all<assets_helper::database_t>();

  nlohmann::json l_json{};
  l_json = l_list;
  co_return in_handle->make_msg(l_json);
}

boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_tree::post(
    http::session_data_ptr in_handle
) {
  auto l_json                                      = in_handle->get_json();
  std::shared_ptr<assets_helper::database_t> l_ptr = std::make_shared<assets_helper::database_t>(
      std::get<nlohmann::json>(in_handle->body_).get<assets_helper::database_t>()
  );

  if (!l_ptr->uuid_parent_.is_nil()) {
    if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(l_ptr->uuid_parent_);
        l_list == 0)
      co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "未找到父节点");
  }
  l_ptr->uuid_id_ = core_set::get_set().get_uuid();
  co_await g_ctx().get<sqlite_database>().install<assets_helper::database_t>(l_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}

boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_tree_instance::put(
    http::session_data_ptr in_handle
) {
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
  auto l_uuid = boost::lexical_cast<uuid>(id_);
  auto l_sql  = g_ctx().get<sqlite_database>();
  if (l_sql.has_assets_tree_assets_link(l_uuid) || l_sql.has_assets_tree_child(l_uuid))
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "该节点有子节点无法删除");
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