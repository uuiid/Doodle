//
// Created by TD on 25-4-22.
//

#pragma once
#include <doodle_lib/http_method/http_jwt_fun.h>

namespace doodle::http::other {

// "api/doodle/key/ji_meng"
DOODLE_HTTP_FUN(key_ji_meng)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
// api/doodle/deepseek/key
DOODLE_HTTP_FUN(deepseek_key)
DOODLE_HTTP_FUN_OVERRIDE(get)
DOODLE_HTTP_FUN_END()
}  // namespace doodle::http::other
