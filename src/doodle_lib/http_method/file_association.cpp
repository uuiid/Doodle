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
#include <doodle_lib/http_method/kitsu.h>

#include <sqlite_orm/sqlite_orm.h>

namespace doodle::http {

boost::asio::awaitable<boost::beast::http::message_generator> doodle_file_association::get(session_data_ptr in_handle) {
  auto l_logger    = in_handle->logger_;
  auto l_sql       = g_ctx().get<sqlite_database>();
  auto l_work_file = l_sql.get_by_uuid<working_file>(id_);
  auto l_entt_ids  = l_sql.impl_->storage_any_.select(
      &working_file_entity_link::entity_id_,
      sqlite_orm::where(sqlite_orm::c(&working_file_entity_link::working_file_id_) == id_)
  );
  if (l_entt_ids.empty())
    in_handle->make_error_code_msg(boost::beast::http::status::not_found, "file not found entity is empty");
  auto l_entt = l_sql.get_by_uuid<entity>(l_entt_ids.front());
  project_minimal l_prj{l_sql.get_by_uuid<project>(l_entt.project_id_)};

  FSys::path l_maya_path, l_ue_path, l_solve_path;
  {
    using namespace sqlite_orm;
    for (auto&& [l_wf, l_t] : l_sql.impl_->storage_any_.select(
             columns(object<working_file>(true), &task::task_type_id_), from<working_file>(),
             join<working_file_entity_link>(
                 on(c(&working_file_entity_link::working_file_id_) == c(&working_file::uuid_id_))
             ),
             join<working_file_task_link>(
                 on(c(&working_file_task_link::working_file_id_) == c(&working_file::uuid_id_))
             ),
             join<task>(on(c(&working_file_task_link::task_id_) == c(&task::uuid_id_))),
             where(c(&working_file_entity_link::entity_id_) == l_entt.uuid_id_)
         )) {
      if (l_t == task_type::get_simulation_id())
        l_solve_path = l_wf.path_;
      else if (l_t == task_type::get_character_id() || l_t == task_type::get_ground_model_id())
        l_ue_path = l_wf.path_;
      else if (l_t == task_type::get_binding_id())
        l_maya_path = l_wf.path_;
    }
  }

  if (l_maya_path.empty() && l_ue_path.empty() && l_solve_path.empty()) {
    l_logger->log(log_loc(), level::info, "file not found");
    co_return in_handle->make_error_code_msg(boost::beast::http::status::not_found, "file not found");
  }
  if (!l_maya_path.empty()) l_maya_path = l_prj.path_ / l_maya_path;
  if (!l_ue_path.empty()) l_ue_path = l_prj.path_ / l_ue_path;
  if (!l_solve_path.empty()) l_solve_path = l_prj.path_ / l_solve_path;

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
  co_return in_handle->make_msg(nlohmann::json::array());
}

}  // namespace doodle::http