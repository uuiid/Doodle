#include "file_association.h"

#include <doodle_core/metadata/kitsu/assets_type.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>

namespace doodle::http {
namespace {
struct key_t : boost::less_than_comparable<key_t> {
  FSys::path path_{};
  std::string version_{};
  explicit key_t(FSys::path in_path, std::string in_version)
      : path_(std::move(in_path)), version_(std::move(in_version)) {}
  friend bool operator==(const key_t& lhs, const key_t& rhs) {
    return lhs.path_ == rhs.path_ && lhs.version_ == rhs.version_;
  }
  friend bool operator<(const key_t& lhs, const key_t& rhs) {
    return std::tie(lhs.path_, lhs.version_) < std::tie(rhs.path_, rhs.version_);
  }
};
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> doodle_file_association_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_logger = in_handle->logger_;

  auto& l_map   = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data();
  if (l_map.contains(in_arg->id_)) {
    auto l_data = l_map.at(in_arg->id_);
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

boost::asio::awaitable<boost::beast::http::message_generator> doodle_file_get::callback_arg(
    session_data_ptr in_handle
) {
  auto l_map = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data();
  nlohmann::json l_json;

  std::set<uuid> l_project_id, l_assets_id;
  std::set<details::assets_type_enum> l_type{};
  for (auto&& l_q : in_handle->url_.params()) {
    if (l_q.has_value && l_q.key == "project_id") {
      l_project_id.emplace(boost::lexical_cast<uuid>(l_q.value));
    } else if (l_q.has_value && l_q.key == "assets_id") {
      l_assets_id.emplace(boost::lexical_cast<uuid>(l_q.value));
    }
  }

  std::map<details::assets_type_enum, uuid> l_type_map{};
  if (auto l_list = g_ctx().get<sqlite_database>().get_all<doodle::metadata::kitsu::assets_type_t>(); !l_list.empty()) {
    for (auto& l_data : l_list) {
      l_type_map[l_data.type_] = l_data.uuid_id_;
      if (l_assets_id.contains(l_data.uuid_id_)) l_type.emplace(l_data.type_);
    }
  }

  std::map<key_t, std::shared_ptr<details::scan_category_data_t>> l_map2{};
  for (auto& l_data : l_map | std::views::values) {
    l_map2.emplace(key_t{l_data->base_path_, l_data->version_name_}, l_data);
  }

  for (auto& l_val : l_map2 | std::views::values) {
    bool l_match{l_project_id.empty() && l_assets_id.empty()};
    if (!l_project_id.empty()) l_match = l_project_id.contains(l_val->project_database_ptr->uuid_id_);
    if (!l_assets_id.empty()) l_match &= l_type.contains(l_val->assets_type_);
    if (!l_match) continue;

    l_json.emplace_back(
        nlohmann::json{
            {"project_id", l_val->project_database_ptr->uuid_id_},
            {"season", l_val->season_.p_int},
            {"number", l_val->number_str_},
            {"name", l_val->name_},
            {"base_path", l_val->base_path_.lexically_proximate(l_val->project_database_ptr->path_)},
            {"version_name", l_val->version_name_},
            {"maya_file", l_val->rig_file_.path_},
            {"ue_file", l_val->ue_file_.path_},
            {"solve_file_", l_val->solve_file_.path_},
            {"assets_type", l_type_map[l_val->assets_type_]},
        }
    );
  }
  co_return in_handle->make_msg(l_json.dump());
}

}  // namespace doodle::http