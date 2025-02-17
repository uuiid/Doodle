//
// Created by TD on 2024/2/27.
//

#pragma once
#include <../../doodle_lib_fwd.h>

#include <../../core/http/http_function.h>
#include <../../core/http/http_route.h>

namespace doodle::http {

void task_info_reg(http_route& in_route);
void task_info_reg_local(http_route& in_route);

}  // namespace doodle::http