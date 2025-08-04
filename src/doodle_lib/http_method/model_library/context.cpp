//
// Created by TD on 25-5-8.
//
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/label.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include "model_library.h"
namespace doodle::http::model_library {
boost::asio::awaitable<boost::beast::http::message_generator> context::get(http::session_data_ptr in_handle) {
  nlohmann::json l_json{};
  l_json["tree_nodes"] = g_ctx().get<sqlite_database>().get_all<assets_helper::database_t>();
  co_return in_handle->make_msg(l_json);
}

}  // namespace doodle::http::model_library