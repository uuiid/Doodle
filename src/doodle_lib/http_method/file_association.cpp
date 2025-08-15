#include "file_association.h"

#include "doodle_core/sqlite_orm/detail/sqlite_database_impl.h"
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/working_file.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> doodle_file_association::get(session_data_ptr in_handle) {
#ifdef DOODLE_SQL_SCAN
  auto l_logger    = in_handle->logger_;
  auto l_sql       = g_ctx().get<sqlite_database>();
  auto l_work_file = l_sql.get_by_uuid<working_file>(id_);
  auto l_entt      = l_sql.get_by_uuid<entity>(l_work_file.entity_id_);
  project_minimal l_prj{l_sql.get_by_uuid<project>(l_entt.project_id_)};

  FSys::path l_maya_path, l_ue_path, l_solve_path;
  {
    using namespace sqlite_orm;
    for (auto&& [l_wf, l_t] : l_sql.impl_->storage_any_.select(
             columns(object<working_file>(true), &task_type::uuid_id_), from<working_file>(),
             join<task>(on(c(&working_file::task_id_) == c(&task::uuid_id_))),
             join<task_type>(on(c(&task::task_type_id_) == c(&task_type::uuid_id_))),
             where(c(&working_file::entity_id_) == l_work_file.entity_id_)
         )) {
      if (l_t == task_type::get_simulation_id())
        l_solve_path = l_wf.path_;
      else if (l_t == task_type::get_character_id() || l_t == task_type::get_ground_model_id())
        l_ue_path = l_wf.path_;
      else if (l_t == task_type::get_binding_id())
        l_maya_path = l_wf.path_;
    }
  }
  co_return in_handle->make_msg(
      nlohmann::json{
          {"maya_file", l_maya_path},
          {"ue_file", l_ue_path},
          {"solve_file_", l_solve_path},
          {"type", convert_assets_type_enum(l_entt.entity_type_id_)},
          {"project", l_prj}
      }
  );
#else
  auto l_logger = in_handle->logger_;

  auto& l_map   = g_ctx().get<std::shared_ptr<scan_win_service_t>>()->get_scan_data();
  if (l_map.contains(id_)) {
    auto l_data = l_map.at(id_);
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
#endif
}

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
boost::asio::awaitable<boost::beast::http::message_generator> doodle_file::get(session_data_ptr in_handle) {
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
  if (auto l_list = g_ctx().get<sqlite_database>().get_all<asset_type>(); !l_list.empty()) {
    for (auto& l_data : l_list) {
      auto l_type_enum        = kitsu::conv_assets_type_enum(l_data.name_);
      l_type_map[l_type_enum] = l_data.uuid_id_;
      if (l_assets_id.contains(l_data.uuid_id_)) l_type.emplace(l_type_enum);
    }
  }
#ifdef DOODLE_SQL_SCAN

  co_return in_handle->make_msg(nlohmann::json::array());
#else

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
#endif
}

}  // namespace doodle::http