//
// Created by TD on 24-9-23.
//

#pragma once
#include <doodle_core/metadata/assets.h>
#include <doodle_core/sqlite_orm/detail/macro.h>

#include <magic_enum.hpp>
#include <sqlite_orm/sqlite_orm.h>

namespace sqlite_orm {
DOODLE_SQLITE_ENUM_TYPE_(::doodle::details::assets_type_enum);
}  // namespace sqlite_orm