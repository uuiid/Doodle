#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

namespace doodle::http {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs, get) {
  person_.check_not_outsourcer();
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_jobs = l_sql.get_all<server_task_info>();
  co_return in_handle->make_msg(nlohmann::json{} = l_jobs);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs_instance, get) {
  person_.check_not_outsourcer();
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_job = l_sql.get_by_uuid<server_task_info>(job_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_job);
}

}  // namespace doodle::http