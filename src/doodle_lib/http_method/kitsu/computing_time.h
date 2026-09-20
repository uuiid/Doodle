#pragma once

#include <doodle_core/metadata/attendance.h>
#include <doodle_core/metadata/user.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
namespace doodle::http {
/// 本次请求对考勤表待写入的改动.
/// 时间钟(create_time_clock)需要读当月考勤, 而"先提交考勤再重算"与"同一事务内提交"的结果必须一致,
/// 因此在提交之前把待写入的改动在内存里叠加到读到的月度考勤上, 这样整条请求只需一次 run_sql.
struct attendance_delta_t {
  /// 这些日期的旧考勤行全部丢弃(该日考勤被整体重写)
  std::vector<chrono::local_days> replaced_days_{};
  /// 这些考勤行被删除
  std::vector<uuid> removed_uuids_{};
  /// 这些考勤行新增或替换(按 uuid_id_ 匹配替换, 匹配不到则追加)
  std::vector<attendance_helper::database_t> upsert_{};
};

/// 读取指定人员指定月份的工时并重算, 只返回更新语句(不执行 SQL).
/// 该月没有工时条目时返回空语句, run_sql 会跳过它.
orm::update_t recomputing_time(
    sqlite_database& in_sql, const uuid& in_person_id, const chrono::year_month& in_year_month,
    const attendance_delta_t& in_delta = {}
);

// "/api/doodle/computing_time/{user_id}/{year_month}"
DOODLE_HTTP_JWT_FUN(computing_time)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_END()

// "/api/doodle/computing_time/{user_id}/{year_month}/add"
DOODLE_HTTP_JWT_FUN(computing_time_add)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()

// "/api/doodle/computing_time/{user_id}/{year_month}/custom"
DOODLE_HTTP_JWT_FUN(computing_time_custom)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_END()

// "/api/doodle/computing_time/{user_id}/{year_month}/sort"
DOODLE_HTTP_JWT_FUN(computing_time_sort)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_END()
// "/api/doodle/computing_time/{user_id}/{year_month}/average"
DOODLE_HTTP_JWT_FUN(computing_time_average)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_END()

// "/api/doodle/computing_time/{user_id}/{year_month}/{task_id}"
DOODLE_HTTP_JWT_FUN(computing_time_patch)
DOODLE_HTTP_FUN_OVERRIDE(patch)
uuid user_id_{};
chrono::year_month year_month_{};
uuid task_id_{};
DOODLE_HTTP_FUN_END()
// "/api/doodle/computing_time/{computing_time_id}"
DOODLE_HTTP_JWT_FUN(computing_time_delete)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http