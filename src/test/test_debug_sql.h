#pragma once

#include <doodle_lib/core/http/http_function.h>

namespace doodle::http {

// /api/doodle/debug/sql
DOODLE_HTTP_FUN(debug_sql)
DOODLE_HTTP_FUN_OVERRIDE(post)
DOODLE_HTTP_ROUTE("/api/doodle/debug/sql")
DOODLE_HTTP_FUN_END()

}  // namespace doodle::http
