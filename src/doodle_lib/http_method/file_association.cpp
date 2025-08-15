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
}

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

  co_return in_handle->make_msg(nlohmann::json::array());
}

}  // namespace doodle::http