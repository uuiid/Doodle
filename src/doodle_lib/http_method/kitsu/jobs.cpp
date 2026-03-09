#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

namespace doodle::http {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs, get) {
  person_.check_not_outsourcer();
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_jobs = l_sql.get_all<server_task_info>();
  co_return in_handle->make_msg(nlohmann::json{} = l_jobs);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs, put) {
  person_.check_not_outsourcer();
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_json = in_handle->get_json().at("id").get<uuid>();
  if (l_json.is_nil()) throw_exception(http_request_error{boost::beast::http::status::bad_request, "缺少计算机 ID"});
  auto l_computer_id = l_sql.get_by_uuid<computer>(l_json);
  if (l_computer_id.status_ != computer_status::online)
    throw_exception(http_request_error{boost::beast::http::status::bad_request, "只能分配给在线的计算机"});
  auto l_jobs = l_sql.get_server_tasks_by_computer_id(l_computer_id.uuid_id_);
  if (l_jobs.empty()) co_return in_handle->make_msg_204();
  auto l_job_ptr       = std::make_shared<server_task_info>(l_jobs.front());
  l_job_ptr->status_   = server_task_info_status::running;
  l_job_ptr->run_time_ = server_task_info::zoned_time{chrono::current_zone(), std::chrono::system_clock::now()};
  co_await g_ctx().get<sqlite_database>().update(l_job_ptr);
  socket_io::broadcast("doodle:task_info:update", nlohmann::json{} = *l_job_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_job_ptr);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs_instance, get) {
  person_.check_not_outsourcer();
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_job = l_sql.get_by_uuid<server_task_info>(job_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_job);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs_instance, put) {
  person_.check_not_outsourcer();
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_job = l_sql.get_by_uuid<server_task_info>(job_id_);
  if (l_job.status_ == server_task_info_status::failed || l_job.status_ == server_task_info_status::completed ||
      l_job.status_ == server_task_info_status::canceled)
    throw_exception(
        http_request_error{boost::beast::http::status::bad_request, "任务在(失败, 完成, 取消)状态下无法修改"}
    );
  auto l_json    = in_handle->get_json();
  auto l_job_ptr = std::make_shared<server_task_info>(l_job);
  l_json.get_to(*l_job_ptr);
  co_await g_ctx().get<sqlite_database>().update(l_job_ptr);

  socket_io::broadcast("doodle:task_info:update", nlohmann::json{} = *l_job_ptr);
  co_return in_handle->make_msg(nlohmann::json{} = *l_job_ptr);
}
}  // namespace doodle::http