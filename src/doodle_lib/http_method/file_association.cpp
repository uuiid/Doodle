#include "file_association.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>

namespace doodle::http {
boost::asio::awaitable<boost::beast::http::message_generator> file_association_get(session_data_ptr in_handle) {
  auto l_logger = in_handle->logger_;

  auto l_id_str = in_handle->capture_->get("uuid");
  boost::uuids::uuid l_id;
  try {
    l_id = boost::lexical_cast<boost::uuids::uuid>(l_id_str);
  } catch (const std::exception& e) {
    l_logger->log(log_loc(), level::err, "error: {}", e.what());
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, e.what());
  } catch (...) {
    l_logger->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::current_exception_diagnostic_information()
    );
  }

  auto& l_map = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data();
  if (l_map.contains(l_id)) {
    auto l_data = l_map.at(l_id);
    nlohmann::json l_json{
        {"maya_file", l_data->rig_file_.path_},
        {"ue_file", l_data->ue_file_.path_},
        {"solve_file_", l_data->solve_file_.path_},
        {"type", l_data->assets_type_},
        {"project", *l_data->project_database_ptr}
    };
    co_return in_handle->make_msg(l_json.dump());
  }
  l_logger->log(log_loc(), level::info, "file not found");
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "file not found");
}
boost::asio::awaitable<boost::beast::http::message_generator> file_list_get(session_data_ptr in_handle) {
  auto l_map = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data();
  nlohmann::json l_json;

  uuid l_project_id, l_assets_id;
  details::assets_type_enum l_type{details::assets_type_enum::other};
  try {
    auto l_q = in_handle->url_.query();
    if (auto l_it = l_q.find("project_id"); l_it != l_q.npos) {
      auto l_str   = l_q.substr(l_it + 11, l_q.find('&', l_it) - l_it - 11);
      l_project_id = boost::lexical_cast<uuid>(l_str);
    }
    if (auto l_it = l_q.find("assets_id"); l_it != l_q.npos) {
      auto l_str  = l_q.substr(l_it + 9, l_q.find('&', l_it) - l_it - 9);
      l_assets_id = boost::lexical_cast<uuid>(l_str);
      if (auto l_t = g_ctx().get<sqlite_database>().get_by_uuid<doodle::metadata::kitsu::task_type_t>(l_assets_id);
          !l_t.empty()) {
        l_type = l_t.front().type_;
      }
    }
  } catch (...) {
    co_return in_handle->make_error_code_msg(
        boost::beast::http::status::bad_request, boost::current_exception_diagnostic_information()
    );
  }
  std::map<details::assets_type_enum, uuid> l_type_map{};
  if (auto l_list = g_ctx().get<sqlite_database>().get_all<doodle::metadata::kitsu::task_type_t>(); !l_list.empty()) {
    for (auto& l_data : l_list) {
      l_type_map[l_data.type_] = l_data.uuid_id_;
    }
  }

  for (auto& l_data : l_map) {
    bool l_match{l_project_id.is_nil() && l_assets_id.is_nil()};
    if (!l_project_id.is_nil()) l_match = l_data.second->project_database_ptr->uuid_id_ == l_project_id;
    if (!l_assets_id.is_nil()) l_match &= l_data.second->assets_type_ == l_type;
    if (!l_match) continue;

    l_json.emplace_back(
        nlohmann::json{
            {"project_id", l_data.second->project_database_ptr->uuid_id_},
            {"season", l_data.second->season_},
            {"number", l_data.second->number_str_},
            {"name", l_data.second->name_},
            {"base_path", l_data.second->base_path_},
            {"version_name", l_data.second->version_name_},
            {"maya_file", l_data.second->rig_file_.path_},
            {"ue_file", l_data.second->ue_file_.path_},
            {"solve_file_", l_data.second->solve_file_.path_},
            {"assets_type", l_type_map[l_data.second->assets_type_]},
        }
    );
  }
  co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "file not found");
}

void reg_file_association_http(http_route& in_route) {
  in_route
      .reg(
          std::make_shared<http_function>(
              boost::beast::http::verb::get, "api/doodle/file_association/{uuid}", file_association_get
          )
      )
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/doodle/file", file_list_get));
}
}  // namespace doodle::http