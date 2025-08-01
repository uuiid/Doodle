//
// Created by TD on 24-11-28.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
// "/api/doodle/tool/version"
DOODLE_HTTP_FUN(doodle_tool_version)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http