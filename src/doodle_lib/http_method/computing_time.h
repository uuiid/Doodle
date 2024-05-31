#pragma once

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>
namespace doodle::http {

void reg_computing_time(http_route& in_route);
}