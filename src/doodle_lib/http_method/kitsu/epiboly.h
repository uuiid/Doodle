//
// Created by TD on 24-12-13.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
namespace doodle::http {
// /api/config
DOODLE_HTTP_FUN(epiboly_config)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/auth/authenticated
DOODLE_HTTP_FUN(epiboly_authenticated)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// /api/data/user/context
DOODLE_HTTP_FUN(epiboly_user_context)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http