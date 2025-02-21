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
  auto l_kitsu_version = g_ctx().get<kitsu_ctx_t>().front_end_root_ / "version.txt";

  if (FSys::exists(l_kitsu_version)) {
    FSys::ifstream l_version_file(l_kitsu_version);
    std::vector<std::string> l_version;
    // getline
    for (std::string l_line; std::getline(l_version_file, l_line);) l_version.emplace_back(std::move(l_line));
    std::ranges::reverse(l_version);
    co_return in_handle->make_msg((nlohmann::json{} = l_version).dump());
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