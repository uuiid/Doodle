#pragma once

#include "doodle_core/metadata/user.h"

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
boost::asio::awaitable<void> recomputing_time(const uuid& in_person_id, const chrono::year_month& in_year_month);
struct computing_time_args {
  uuid user_id_{};
  chrono::year_month year_month_{};
};

// "/api/doodle/computing_time/{user_id}/{year_month}"
DOODLE_HTTP_FUN(computing_time)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_END()

// "/api/doodle/computing_time/{user_id}/{year_month}/add"
DOODLE_HTTP_FUN(computing_time_add)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_FUN_END()

// "/api/doodle/computing_time/{user_id}/{year_month}/custom"
DOODLE_HTTP_FUN(computing_time_custom)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_END()

// "/api/doodle/computing_time/{user_id}/{year_month}/sort"
DOODLE_HTTP_FUN(computing_time_sort)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_END()
// "/api/doodle/computing_time/{user_id}/{year_month}/average"
DOODLE_HTTP_FUN(computing_time_average)
DOODLE_HTTP_FUN_OVERRIDE(post)
uuid user_id_{};
chrono::year_month year_month_{};
DOODLE_HTTP_FUN_END()

// "/api/doodle/computing_time/{user_id}/{year_month}/{task_id}"
DOODLE_HTTP_FUN(computing_time_patch)
DOODLE_HTTP_FUN_OVERRIDE(patch)
uuid user_id_{};
chrono::year_month year_month_{};
uuid task_id_{};
DOODLE_HTTP_FUN_END()
// "/api/doodle/computing_time/{computing_time_id}"
DOODLE_HTTP_FUN(computing_time_delete)
DOODLE_HTTP_FUN_OVERRIDE(delete_)
uuid id_{};
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http