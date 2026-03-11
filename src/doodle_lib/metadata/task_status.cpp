#include "task_status.h"

#include <doodle_lib/sqlite_orm/sqlite_database.h>

namespace doodle::task_status_ns {
void check_retake_capping(const task_status& in_task_status, const task& in_task) {
  if (!in_task_status.is_retake_) return;

  auto l_prj = get_sqlite_database().get_by_uuid<project>(in_task.project_id_);
  if (l_prj.max_retakes_ <= 0) return;
  if (in_task.retake_count_ >= l_prj.max_retakes_)
    throw_exception(
        http_request_error{
            boost::beast::http::status::bad_request, "任务 {} 已经超过最大重拍次数 {}", in_task.name_,
            l_prj.max_retakes_
        }
    );
}
}  // namespace doodle::task_status_ns