#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/metadata/kitsu_ctx_t.h"
#include <doodle_core/metadata/computer.h>
#include <doodle_core/metadata/server_task_info.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/socket_io.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/core/socket_io/websocket_impl.h>
#include <doodle_lib/http_method/kitsu/computers.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/asio/awaitable.hpp>

#include <filesystem>

namespace doodle::http {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs, get) {
  person_.check_not_outsourcer();
  auto l_sql  = get_sqlite_database();
  auto l_jobs = l_sql.get_all<server_task_info>();
  for (auto& l_job : l_jobs) {
    if (l_job.status_ == server_task_info_status::failed)
      l_job.get_last_line_log(g_ctx().get<kitsu_ctx_t>().get_jobs_logs_file(l_job.uuid_id_));
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_jobs);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_jobs_log, get) {
  person_.check_not_outsourcer();
  auto l_jobs_path = g_ctx().get<kitsu_ctx_t>().get_jobs_logs_file(job_id_);
  if (!FSys::exists(l_jobs_path)) co_return in_handle->make_msg_204();
  co_return in_handle->make_msg(l_jobs_path, "text/plain; charset=utf-8");  // utf-8编码的文本文件
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_jobs_log, put) {
  person_.check_not_outsourcer();
  auto l_jobs_path = g_ctx().get<kitsu_ctx_t>().get_jobs_logs_file(job_id_);
  auto l_string    = in_handle->get_string();
  if (auto l_p = l_jobs_path.parent_path(); !FSys::exists(l_p)) FSys::create_directories(l_p);
  FSys::ofstream{l_jobs_path, std::ios::out | std::ios::binary | std::ios::app} << l_string;
  co_return in_handle->make_msg_204();
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs_instance, get) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  auto l_job = l_sql.get_by_uuid<server_task_info>(job_id_);
  l_job.get_last_line_log(g_ctx().get<kitsu_ctx_t>().get_jobs_logs_file(job_id_));
  co_return in_handle->make_msg(nlohmann::json{} = l_job);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs_instance, put) {
  person_.check_not_outsourcer();
  auto l_sql     = get_sqlite_database();
  auto l_job     = l_sql.get_by_uuid<server_task_info>(job_id_);
  auto l_json    = in_handle->get_json();
  auto l_job_ptr = std::make_shared<server_task_info>(l_job);
  l_json.get_to(*l_job_ptr);
  co_await get_sqlite_database().update(l_job_ptr);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 更新任务 job_id {} status {} -> {} ", person_.person_.email_,
      person_.person_.get_full_name(), job_id_, l_job.status_, l_job_ptr->status_
  );

  if (l_job_ptr->run_computer_id_ != l_job.run_computer_id_) {
    SPDLOG_LOGGER_WARN(
        g_logger_ctrl().get_http(), "任务 {} 分配的计算机由 {} 变更为 {}, 将尝试让新计算机执行任务",
        l_job_ptr->uuid_id_, l_job.run_computer_id_, l_job_ptr->run_computer_id_
    );
    co_await computers_assign_task::get_instance().run_next_task();
  }
  socket_io::broadcast(socket_io::server_task_info_update_broadcast_t{.server_task_info_id_ = l_job_ptr->uuid_id_});
  co_return in_handle->make_msg(nlohmann::json{} = *l_job_ptr);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_jobs_instance, delete_) {
  person_.check_not_outsourcer();
  auto l_sql = get_sqlite_database();
  auto l_job = l_sql.get_by_uuid<server_task_info>(job_id_);
  co_await l_sql.remove<server_task_info>(job_id_);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除任务 job_id {} status {} ", person_.person_.email_,
      person_.person_.get_full_name(), job_id_, l_job.status_
  );
  socket_io::broadcast(socket_io::server_task_info_delete_broadcast_t{.server_task_info_id_ = job_id_});
  co_return in_handle->make_msg_204();
}

}  // namespace doodle::http