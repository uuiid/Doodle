//
// Created by TD on 25-4-30.
//
#include "task_status.h"

#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/task.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
namespace doodle {
void task_status::check_retake_capping(const task& in_task) {
  if (!is_retake_) return;

  auto l_prj = g_ctx().get<sqlite_database>().get_by_uuid<project>(in_task.project_id_);
  if (l_prj.max_retakes_ <= 0) return;
  if (in_task.retake_count_ >= l_prj.max_retakes_)
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request,
            fmt::format("任务 {} 已经超过最大重拍次数 {}", in_task.name_, l_prj.max_retakes_)
        }
    );
}

}  // namespace doodle