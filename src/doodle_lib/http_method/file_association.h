#pragma once

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {

// /api/doodle/file_association/{uuid}
DOODLE_HTTP_JWT_FUN(doodle_file_association)
DOODLE_HTTP_FUN_OVERRIDE(get)
uuid id_{};
DOODLE_HTTP_FUN_END()
// "/api/doodle/file"
DOODLE_HTTP_JWT_FUN(doodle_file)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http