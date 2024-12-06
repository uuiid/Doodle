//
// Created by TD on 24-12-3.
//

#include "local_setting.h"

#include "doodle_core/sqlite_orm/sqlite_database.h"
#include <doodle_core/core/authorization.h>
#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

namespace doodle::http {

namespace {
boost::asio::awaitable<boost::beast::http::message_generator> get_local_setting(session_data_ptr in_handle) {
  auto l_a = authorization{core_set::get_set().authorize_}.is_expire();
  co_return in_handle->make_msg(
      nlohmann::json{{"maya_parallel_quantity", core_set::get_set().p_max_thread}, {"authorize", l_a}}.dump()
  );
}
boost::asio::awaitable<boost::beast::http::message_generator> set_local_setting(session_data_ptr in_handle) {
  if (in_handle->content_type_ != http::detail::content_type::application_json) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "不是json请求");
  }
  try {
    auto& l_json                     = std::get<nlohmann::json>(in_handle->body_);
    core_set::get_set().p_max_thread = l_json["maya_parallel_quantity"];
    g_ctx().get<maya_ctx>().queue_->set_limit(core_set::get_set().p_max_thread);
    if (l_json.contains("authorize") && l_json["authorize"].is_string())
      if (auto& l_a = l_json["authorize"]; l_a.is_string() && authorization{l_a.get<std::string>()}.is_expire())
        core_set::get_set().authorize_ = l_a.get<std::string>();

    core_set::get_set().save();
  } catch (...) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "无效的json");
  }
  auto l_a = authorization{core_set::get_set().authorize_}.is_expire();

  co_return in_handle->make_msg(
      nlohmann::json{{"maya_parallel_quantity", core_set::get_set().p_max_thread}, {"authorize", l_a}}.dump()
  );
}
}  // namespace
void local_setting_reg(http_route& in_route) {
  g_ctx().emplace<maya_ctx>();
  in_route
      .reg(
          std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/local_setting", get_local_setting)
      )
      .reg(
          std::make_shared<http_function>(boost::beast::http::verb::post, "api/doodle/local_setting", set_local_setting)

      )

      ;
}
}  // namespace doodle::http