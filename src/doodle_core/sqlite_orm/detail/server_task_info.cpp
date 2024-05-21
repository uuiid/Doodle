//
// Created by TD on 2024/3/13.
//
#include "all.h"
//
#include <doodle_core/core/core_sql.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/sqlite3/sqlite3.h>
#include <doodle_core/sqlite_orm/sqlite_base.h>
//
#include <doodle_core/metadata/server_task_info.h>
//
#include <boost/preprocessor.hpp>

#include <entt/entt.hpp>
#include <sqlpp11/ppgen.h>
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

namespace doodle::snapshot {

void reg_server_task_info() {}

}  // namespace doodle::snapshot