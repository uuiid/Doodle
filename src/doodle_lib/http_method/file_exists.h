//
// Created by TD on 24-4-22.
//

#pragma once
#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/http_route.h>

namespace doodle::http {
void file_exists_reg(http_route& in_route);
} // namespace doodle::http