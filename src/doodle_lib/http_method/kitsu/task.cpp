//
// Created by TD on 24-8-20.
//

#include "task.h"

#include <doodle_core/metadata/project.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {

namespace {

boost::asio::awaitable<boost::beast::http::message_generator> get_task_info(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};

  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }

  auto l_json = nlohmann::json::parse(l_res.body());
  try {
    auto l_user_data = l_json["entity"]["data"];
    scan::scan_key_t l_key{
        .dep_          = conv_assets_type_enum(l_json["dep"]["name"]),
        .season_       = season{l_user_data["gui_dang"].get<std::int32_t>()},
        .project_      = find_project(l_json["project"]["name"]),
        .number_       = l_user_data.contains("bian_hao") ? l_user_data["bian_hao"].get<std::string>() : std::string{},
        .name_         = l_user_data["pin_yin_ming_cheng"].get<std::string>(),
        .version_name_ = l_user_data.contains("ban_ben") ? l_user_data["ban_ben"].get<std::string>() : std::string{},
    };
    FSys::path l_maya_path{};
    FSys::path l_ue_path{};
    auto& l_map = g_ctx().get<scan_win_service_t>().get_scan_data_key();
    if (l_map.contains(l_key)) {
      l_maya_path = l_map.at(l_key)->rig_file_.path_;
      l_ue_path   = l_map.at(l_key)->ue_file_.path_;
    }
    l_json["file_exist"] = FSys::exists(l_maya_path) && FSys::exists(l_ue_path);
    l_res.body()         = l_json.dump();
  } catch (...) {
    l_json["file_exist"] = false;
    l_res.body()         = l_json.dump();
    co_return std::move(l_res);
  }
  co_return std::move(l_res);
}

}  // namespace
void kitsu_task_reg(http_route& in_http_route) {
  in_http_route

      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/task/{task_id}/full", get_task_info
      ))

      ;
}
}  // namespace doodle::http::kitsu