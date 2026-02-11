//
// Created by TD on 24-10-15.
//

#include "doodle_core/metadata/label.h"
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include "doodle_lib/core/http/http_function.h"
#include <doodle_lib/http_method/kitsu.h>

#include "boost/lexical_cast.hpp"

#include "model_library.h"
#include <jwt-cpp/jwt.h>
namespace doodle::http::model_library {

boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets::get(
    http::session_data_ptr in_handle
) {
  person_.check_user();
  auto l_list = g_ctx().get<sqlite_database>().get_all<assets_file_helper::database_t>();
  nlohmann::json l_json{};
  l_json = l_list;
  co_return in_handle->make_msg(l_json);
}
boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets::post(
    http::session_data_ptr in_handle
) {
  person_.check_supervisor();
  auto l_json                                           = in_handle->get_json();
  std::shared_ptr<assets_file_helper::database_t> l_ptr = std::make_shared<assets_file_helper::database_t>();
  l_json.get_to(*l_ptr);
  // 额外添加标签
  l_json["parents"].get_to(l_ptr->uuid_parents_);
  for (auto&& i : l_ptr->uuid_parents_)
    if (auto l_list = g_ctx().get<sqlite_database>().uuid_to_id<assets_helper::database_t>(i); l_list == 0)
      co_return in_handle->make_error_code_msg(
          boost::beast::http::status::not_found, fmt::format("未找到父节点 {}", i)
      );

  co_await g_ctx().get<sqlite_database>().install<assets_file_helper::database_t>(l_ptr);
  auto l_link = std::make_shared<std::vector<assets_file_helper::link_parent_t>>();
  for (auto&& i : l_ptr->uuid_parents_)
    l_link->emplace_back(assets_file_helper::link_parent_t{.assets_type_uuid_ = i, .assets_uuid_ = l_ptr->uuid_id_});
  co_await g_ctx().get<sqlite_database>().install_range<assets_file_helper::link_parent_t>(l_link);

  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_instance::put(
    http::session_data_ptr in_handle
) {
  person_.check_supervisor();
  std::shared_ptr<assets_file_helper::database_t> const l_ptr = std::make_shared<assets_file_helper::database_t>(
      g_ctx().get<sqlite_database>().get_by_uuid<assets_file_helper::database_t>(id_)
  );
  in_handle->get_json().get_to(*l_ptr);
  co_await g_ctx().get<sqlite_database>().update<assets_file_helper::database_t>(l_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}

boost::asio::awaitable<boost::beast::http::message_generator> model_library_assets_instance::delete_(
    http::session_data_ptr in_handle
) {
  person_.check_supervisor();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 资产库文件 {} ", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );
  co_await g_ctx().get<sqlite_database>().remove<assets_file_helper::database_t>(id_);
  co_return in_handle->make_msg(nlohmann::json{});
}

}  // namespace doodle::http::model_library