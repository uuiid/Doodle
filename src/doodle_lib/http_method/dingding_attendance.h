#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {

struct dingding_attendance_args {

};
// "/api/doodle/attendance/{user_id}/{date}"
DOODLE_HTTP_JWT_FUN(dingding_attendance_get)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid user_id_;
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_END()
//"/api/doodle/attendance/{user_id}"
DOODLE_HTTP_JWT_FUN(dingding_attendance_create_post)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// "/api/doodle/attendance/{user_id}/custom"
DOODLE_HTTP_JWT_FUN(dingding_attendance_id_custom)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid id_{};
DOODLE_HTTP_FUN_END()
// "/api/doodle/attendance/custom/{id}"
DOODLE_HTTP_JWT_FUN(dingding_attendance_custom)
DOODLE_HTTP_FUN_OVERRIDE(put)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http