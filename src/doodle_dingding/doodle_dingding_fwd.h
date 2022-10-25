//
// Created by TD on 2022/9/7.
//

#pragma once

#include <doodle_core/doodle_core_fwd.h>

#include <doodle_dingding/configure/doodle_dingding_export.h>

namespace doodle::business {
class work_clock;
}

namespace doodle::dingding {

namespace department_ns {
class department_query;
class user_to_dep_query;
}  // namespace department_ns

namespace attendance {
namespace query {
class get_day_data;
class get_update_data;

}  // namespace query

class attendance_record;
class attendance;
}  // namespace attendance

class department;

namespace user_dd_ns {
class dep_query;
class find_by_mobile;
class get_user_info;
}  // namespace user_dd_ns

class user_dd;
}  // namespace doodle::dingding