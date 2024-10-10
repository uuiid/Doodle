//
// Created by TD on 24-8-20.
//

#include "task.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {

namespace {

boost::asio::awaitable<boost::beast::http::message_generator> get_task_info_full(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};

  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }

  auto l_json = nlohmann::json::parse(l_res.body());
  bool l_file_exist{};
  try {
    auto l_task_type_name = l_json["task_type"]["name"];
    if (l_task_type_name == "角色" || l_task_type_name == "地编模型" || l_task_type_name == "绑定") {
      auto l_user_data = l_json["entity"]["data"];
      scan::scan_key_t l_key{
          .dep_     = conv_assets_type_enum(l_json["dep"]["name"]),
          .season_  = season{l_user_data["gui_dang"].get<std::int32_t>()},
          .project_ = find_project(l_json["project"]["name"]).uuid_id_,
          .number_  = l_user_data.contains("bian_hao") ? l_user_data["bian_hao"].get<std::string>() : std::string{},
          .name_    = l_user_data["pin_yin_ming_cheng"].get<std::string>(),
          .version_name_ = l_user_data.contains("ban_ben") ? l_user_data["ban_ben"].get<std::string>() : std::string{},
      };
      FSys::path l_maya_path{};
      FSys::path l_ue_path{};
      auto& l_map = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data_key();
      if (l_map.contains(l_key)) {
        l_maya_path = l_map.at(l_key)->rig_file_.path_;
        l_ue_path   = l_map.at(l_key)->ue_file_.path_;
      }
      l_file_exist = FSys::exists(l_maya_path) && FSys::exists(l_ue_path);
    } else {
      l_file_exist = true;
    }
  } catch (...) {
    l_file_exist = false;
  }
  l_json["file_exist"] = l_file_exist;
  l_res.body()         = l_json.dump();
  l_res.prepare_payload();
  co_return std::move(l_res);
}
boost::asio::awaitable<boost::beast::http::message_generator> get_task_with_tasks(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};

  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return std::move(l_res);
  }
  try {
    auto l_json   = nlohmann::json::parse(std::as_const(l_res).body());
    auto l_prj_id = get_url_project_id(in_handle->url_);
    uuid l_prj_uuid{};
    if (auto l_p = g_ctx().get<sqlite_database>().get_by_kitsu_uuid<project_helper::database_t>(l_prj_id); !l_p.empty())
      l_prj_uuid = l_p.front().uuid_id_;

    auto& l_map = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data_key();
    for (auto&& l_json_entt : l_json) {
      auto l_user_data       = l_json_entt["data"];
      auto l_asset_type_name = l_json_entt["asset_type_name"];
      for (auto&& l_json_task : l_json_entt["tasks"]) {
        bool l_file_exist{};
        if (auto l_p = g_ctx().get<sqlite_database>().get_by_kitsu_uuid<metadata::kitsu::task_type_t>(
                l_json_task["task_type_id"].get<uuid>()
            );
            !l_p.empty() && l_p.front().use_chick_files) {
          scan::scan_key_t l_key{
              .dep_ = conv_assets_type_enum(l_asset_type_name),
              .season_ =
                  season{l_user_data.contains("gui_dang") ? std::stoi(l_user_data["gui_dang"].get<std::string>()) : 0},
              .project_ = l_prj_uuid,
              .number_  = l_user_data.contains("bian_hao") ? l_user_data["bian_hao"].get<std::string>() : std::string{},
              .name_ = l_user_data.contains("pin_yin_ming_cheng") ? l_user_data["pin_yin_ming_cheng"].get<std::string>()
                                                                  : std::string{},
              .version_name_ =
                  l_user_data.contains("ban_ben") ? l_user_data["ban_ben"].get<std::string>() : std::string{},
          };
          FSys::path l_maya_path{};
          FSys::path l_ue_path{};
          if (l_map.contains(l_key)) {
            l_maya_path = l_map.at(l_key)->rig_file_.path_;
            l_ue_path   = l_map.at(l_key)->ue_file_.path_;
          }
          // l_file_exist = FSys::exists(l_maya_path) && FSys::exists(l_ue_path);
          l_file_exist = l_map.contains(l_key);
        } else {
          l_file_exist = true;
        }
        l_json_task["file_exist"] = l_file_exist;
      }
    }
    l_res.body() = l_json.dump();
    l_res.prepare_payload();
  } catch (...) {
    in_handle->logger_->error("api/data/assets/with-tasks {}", boost::current_exception_diagnostic_information());
  }
  co_return std::move(l_res);
}
}  // namespace
void task_reg(http_route& in_http_route) {
  in_http_route

      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/doodle/task/{task_id}/full", get_task_info_full
      ))
      .reg(std::make_shared<http_function>(
          boost::beast::http::verb::get, "api/data/assets/with-tasks", get_task_with_tasks
      ))

      ;
}
}  // namespace doodle::http::kitsu