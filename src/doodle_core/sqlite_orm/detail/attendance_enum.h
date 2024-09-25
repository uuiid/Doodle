//
// Created by TD on 24-9-25.
//

#pragma once
#include <doodle_core/metadata/attendance.h>
#include <doodle_core/sqlite_orm/detail/macro.h>

#include <magic_enum.hpp>
#include <sqlite_orm/sqlite_orm.h>
namespace sqlite_orm {
DOODLE_SQLITE_ENUM_TYPE_(::doodle::attendance_helper::att_enum);
}  // namespace sqlite_orm
