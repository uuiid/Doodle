//
// Created by TD on 24-12-3.
//

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

#include "local.h"

namespace doodle::http::local {

boost::asio::awaitable<boost::beast::http::message_generator> local_setting_get::callback_arg(
    session_data_ptr in_handle
) {
  FSys::path l_maya_path{};
  try {
    l_maya_path = maya_exe_ns::find_maya_path();
  } catch (const doodle_error& e) {
    default_logger_raw()->error(e.what());
  }
  co_return in_handle->make_msg(
      nlohmann::json{
          {"maya_parallel_quantity", core_set::get_set().p_max_thread},
          {"authorize", g_ctx().get<authorization>().is_expire()},
          {"maya_path", l_maya_path},
          {"UE_path", core_set::get_set().ue4_path},
          {"UE_version", core_set::get_set().ue4_version},
      }
          .dump()
  );
}

boost::asio::awaitable<boost::beast::http::message_generator> local_setting_post::callback_arg(
    session_data_ptr in_handle
) {
  if (in_handle->content_type_ != http::detail::content_type::application_json) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "不是json请求");
  }

  auto& l_set        = core_set::get_set();
  auto& l_json       = std::get<nlohmann::json>(in_handle->body_);
  l_set.p_max_thread = l_json["maya_parallel_quantity"];
  g_ctx().get<maya_ctx>().queue_->set_limit(l_set.p_max_thread);
  auto& l_au = g_ctx().get<authorization>();
  if (l_json.contains("authorize") && l_json["authorize"].is_string())
    if (auto& l_a = l_json["authorize"];
        l_a.is_string() && (l_au.load_authorization_data(l_a.get<std::string>()), l_au.is_expire()))
      l_set.authorize_ = l_a.get<std::string>();

  if (l_json.contains("UE_path")) l_set.ue4_path = l_json["UE_path"].get<std::string>();
  if (l_json.contains("UE_version")) l_set.ue4_version = l_json["UE_version"].get<std::string>();
  // D:\Program Files\Epic Games\UE_5.4\Engine\Binaries\Win64\UnrealEditor.exe
  if (!l_set.ue4_path.empty() && !FSys::exists(l_set.ue4_path / "Engine" / "Binaries" / "Win64" / "UnrealEditor.exe"))
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "UE4路径不正确"});

  l_set.save();
  FSys::path l_maya_path{};
  try {
    l_maya_path = maya_exe_ns::find_maya_path();
  } catch (const doodle_error& e) {
    default_logger_raw()->error(e.what());
  }
  co_return in_handle->make_msg(
      nlohmann::json{
          {"maya_parallel_quantity", l_set.p_max_thread},
          {"authorize", g_ctx().get<authorization>().is_expire()},
          {"maya_path", l_maya_path},
          {"UE_path", l_set.ue4_path},
          {"UE_version", l_set.ue4_version},

      }
          .dump()
  );
}

}  // namespace doodle::http::local