//
// Created by TD on 25-4-30.
//
#include "task_status.h"

#include "doodle_core_fwd.h"
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
const uuid& task_status::get_completed() {
  // 4ffc748e-4e58-4336-ba83-51910253514e
  static const uuid g_completed_id{
      {0x4f, 0xfc, 0x74, 0x8e, 0x4e, 0x58, 0x43, 0x36, 0xba, 0x83, 0x51, 0x91, 0x02, 0x53, 0x51, 0x4e}
  };
  return g_completed_id;
}

const uuid& task_status::get_nearly_completed() {
  // 1cbeaa6b-7825-4bcd-8878-6f9fb05ac493
  static const uuid g_nearly_completed_id{
      {0x1c, 0xbe, 0xaa, 0x6b, 0x78, 0x25, 0x4b, 0xcd, 0x88, 0x78, 0x6f, 0x9f, 0xb0, 0x5a, 0xc4, 0x93}
  };
  return g_nearly_completed_id;
}

const uuid& task_status::get_to_do() {
  // 9704a092-76ac-406d-89e5-1dd862e03e82
  static const uuid g_to_do_id{
      {0x97, 0x04, 0xa0, 0x92, 0x76, 0xac, 0x40, 0x6d, 0x89, 0xe5, 0x1d, 0xd8, 0x62, 0xe0, 0x3e, 0x82}
  };
  return g_to_do_id;
}

}  // namespace doodle