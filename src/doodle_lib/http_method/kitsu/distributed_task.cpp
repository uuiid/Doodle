#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

namespace doodle::http {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_projects_shots_run_ue_assembly_subtasks, post) {
  auto l_ptr              = std::make_shared<server_task_info>();
  l_ptr->type_            = server_task_info_type::auto_light;
  l_ptr->submit_time_     = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  l_ptr->run_computer_id_ = boost::uuids::nil_uuid();

  auto l_json             = in_handle->get_json();
  l_json.get_to(*l_ptr);
  if (l_ptr->name_.empty()) l_ptr->name_ = fmt::to_string(l_ptr->uuid_id_);
  co_await g_ctx().get<sqlite_database>().install(l_ptr);

  socket_io::broadcast("doodle:task_info:create", nlohmann::json{} = *l_ptr);
  co_return in_handle->make_msg((nlohmann::json{} = *l_ptr).dump());
}
}  // namespace doodle::http