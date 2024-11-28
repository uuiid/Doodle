//
// Created by TD on 24-11-28.
//

#include "tool_version.h"

#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http {

namespace {

boost::asio::awaitable<boost::beast::http::message_generator> get_tool_version(session_data_ptr in_handle) {
  auto l_kitsu_version = g_ctx().get<kitsu_ctx_t>().root_ / "version.txt";

  if (FSys::exists(l_kitsu_version)) {
    FSys::ifstream l_version_file(l_kitsu_version);
    std::string l_version{std::istreambuf_iterator<char>(l_version_file), std::istreambuf_iterator<char>()};
    co_return in_handle->make_msg(nlohmann::json{{"version", l_version}}.dump());
  }
  co_return in_handle->make_msg(nlohmann::json{});
}
}  // namespace
void tool_version_reg(http_route& in_route) {
  in_route.reg(
      std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/tool/version", get_tool_version)
  );
}
}  // namespace doodle::http