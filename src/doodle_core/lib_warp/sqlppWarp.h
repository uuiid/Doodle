//
// Created by TD on 2021/6/17.
//

#pragma once
#include <doodle_core/lib_warp/sqlite3/insert.h>
#include <doodle_core/lib_warp/sqlite3/on_conflict.h>
#include <doodle_core/lib_warp/sqlite3/on_conflict_do_nothing.h>
#include <doodle_core/lib_warp/sqlite3/on_conflict_do_update.h>
#include <doodle_core/lib_warp/sqlite3/result_field.h>
#include <doodle_core/lib_warp/sqlite3/returning.h>
#include <doodle_core/lib_warp/sqlite3/returning_column_list.h>
#include <doodle_core/lib_warp/sqlite3/serializer.h>
#include <doodle_core/lib_warp/sqlite3/update.h>

#include <sqlpp11/connection.h>
// 添加数据库连接项
namespace sqlpp {
template <typename ConnectionBase>
class normal_connection;
namespace sqlite3 {
class connection_base;
using connection = normal_connection<sqlite3::connection_base>;
}  // namespace sqlite3
}  // namespace sqlpp
