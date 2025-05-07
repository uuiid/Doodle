//
// Created by TD on 24-8-20.
//

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> data_user_tasks_get::callback(
    session_data_ptr in_handle
) {
  auto l_ptr = get_person(in_handle);
  auto& sql  = g_ctx().get<sqlite_database>();
  auto l_p1  = sql.get_person_tasks(l_ptr->person_);
  co_return in_handle->make_msg((nlohmann::json{} = l_p1).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_user_done_tasks_get::callback(
    session_data_ptr in_handle
) {
  auto l_ptr = get_person(in_handle);
  auto& sql  = g_ctx().get<sqlite_database>();
  auto l_p1  = sql.get_person_tasks(l_ptr->person_, true);
  co_return in_handle->make_msg((nlohmann::json{} = l_p1).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> tasks_to_check_get::callback(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);

  switch (l_ptr->person_.role_) {
    case person_role_type::admin:
    case person_role_type::supervisor:
    case person_role_type::manager:
      break;

    case person_role_type::user:
    case person_role_type::client:
    case person_role_type::vendor:
      co_return in_handle->make_msg("[]"s);
      break;
  }

  auto& sql = g_ctx().get<sqlite_database>();
  auto l_p1 = sql.get_preson_tasks_to_check(l_ptr->person_);
  co_return in_handle->make_msg((nlohmann::json{} = l_p1).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> tasks_comments_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  auto l_task_id = from_uuid_str(in_handle->capture_->get("task_id"));

  auto& sql      = g_ctx().get<sqlite_database>();
  auto l_p1      = sql.get_comments(l_task_id);
  co_return in_handle->make_msg((nlohmann::json{} = l_p1).dump());
}

}  // namespace doodle::http

namespace doodle::http::kitsu {

namespace {

boost::asio::awaitable<boost::beast::http::message_generator> get_task_info_full(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};

  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }

  bool l_file_exist{};
  FSys::path l_path{};
  auto l_json = nlohmann::json::parse(l_res.body());
  try {
    auto l_task_type_name = l_json["task_type"]["name"];
    if (l_task_type_name == "角色" || l_task_type_name == "地编模型" || l_task_type_name == "绑定") {
      auto l_user_data = l_json["entity"]["data"];
      if (l_user_data.contains("gui_dang") &&            //
          l_user_data.contains("bian_hao") &&            //
          l_user_data.contains("pin_yin_ming_cheng") &&  //
          l_user_data.contains("ban_ben")) {
        scan::scan_key_t l_key{
            .dep_          = conv_assets_type_enum(l_json["dep"]["name"]),
            .season_       = season{std::stoi(l_user_data["gui_dang"].get<std::string>())},
            .project_      = find_project(l_json["project"]["name"]).uuid_id_,
            .number_       = l_user_data["bian_hao"].get<std::string>(),
            .name_         = l_user_data["pin_yin_ming_cheng"].get<std::string>(),
            .version_name_ = l_user_data["ban_ben"].get<std::string>(),
        };
        auto& l_map  = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data_key();
        l_file_exist = l_map.contains(l_key);
        if (l_file_exist)
          if (l_task_type_name == "绑定")
            l_path = l_map.at(l_key)->rig_file_.path_;
          else
            l_path = l_map.at(l_key)->ue_file_.path_;
      } else
        l_file_exist = false;
    } else {
      l_file_exist = true;
    }
  } catch (...) {
    l_file_exist = false;
  }
  l_json["file_exist"] = l_file_exist;
  l_json["path"]       = l_path;
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

    auto& l_map   = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data_key();
    for (auto&& l_json_entt : l_json) {
      auto l_user_data       = l_json_entt["data"];
      auto l_asset_type_name = l_json_entt["asset_type_name"];
      for (auto&& l_json_task : l_json_entt["tasks"]) {
        bool l_file_exist{};
        FSys::path l_path{};
        if (auto l_p = g_ctx().get<sqlite_database>().get_by_uuid<metadata::kitsu::task_type_t>(
                l_json_task["task_type_id"].get<uuid>()
            );
            l_p.use_chick_files) {
          if (l_user_data.contains("gui_dang") &&  //
              l_user_data.contains("pin_yin_ming_cheng")) {
            std::int32_t l_gui_dang{};
            if (l_user_data["gui_dang"].is_number())
              l_gui_dang = l_user_data["gui_dang"].get<std::int32_t>();
            else if (l_user_data["gui_dang"].is_string() && !l_user_data["gui_dang"].get<std::string>().empty())
              l_gui_dang = std::stoi(l_user_data["gui_dang"].get<std::string>());
            auto l_pin_yin_ming_cheng = l_user_data["pin_yin_ming_cheng"].get<std::string>();
            scan::scan_key_t l_key{
                .dep_          = conv_assets_type_enum(l_asset_type_name),
                .season_       = season{l_gui_dang},
                .project_      = l_prj_id,
                .number_       = l_user_data.contains("bian_hao") && l_user_data["bian_hao"].is_string()
                                     ? l_user_data["bian_hao"].get<std::string>()
                                     : std::string{},
                .name_         = l_pin_yin_ming_cheng,
                .version_name_ = l_user_data.contains("ban_ben") && l_user_data["ban_ben"].is_string()
                                     ? l_user_data["ban_ben"].get<std::string>()
                                     : std::string{},
            };
            l_file_exist = l_map.contains(l_key);
            if (l_file_exist)
              if (l_p.name_ == "绑定")
                l_path = l_map.at(l_key)->rig_file_.path_;
              else
                l_path = l_map.at(l_key)->ue_file_.path_;
          } else
            l_file_exist = false;
        } else
          l_file_exist = true;

        l_json_task["file_exist"] = l_file_exist;
        l_json_task["path"]       = l_path;
      }
    }
    l_res.body() = l_json.dump();
    l_res.prepare_payload();
  } catch (...) {
    in_handle->logger_->error("api/data/assets/with-tasks {}", boost::current_exception_diagnostic_information());
  }
  co_return std::move(l_res);
}

boost::asio::awaitable<boost::beast::http::message_generator> create_task(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};

  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);

  if (l_ec) {
    co_return std::move(l_res);
  }
  if (auto l_json = nlohmann::json::parse(l_res.body()); l_json.contains("id"))
    g_ctx().get<cache_manger>().erase(l_json["id"].get<uuid>());
  co_return std::move(l_res);
}
boost::asio::awaitable<boost::beast::http::message_generator> create_task2(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  l_request.body()   = std::get<nlohmann::json>(in_handle->body_).dump();

  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);

  if (l_ec) {
    co_return std::move(l_res);
  }
  if (auto l_json = nlohmann::json::parse(l_res.body()); l_json.contains("id"))
    g_ctx().get<cache_manger>().erase(l_json["id"].get<uuid>());
  co_return std::move(l_res);
}

}  // namespace
void task_reg(http_route& in_http_route) {
  in_http_route

      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/doodle/task/{task_id}/full", get_task_info_full
          )
      )
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/data/assets/with-tasks", get_task_with_tasks
          )
      )
      .reg(std::make_shared<http_function>(boost::beast::http::verb::post, "api/data/tasks", create_task))
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::post,
              "api/actions/projects/{project_id}/task-types/{task_type_id}/assets/create-tasks", create_task2
          )
      );
}
}  // namespace doodle::http::kitsu