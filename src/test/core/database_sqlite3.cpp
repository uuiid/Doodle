//
// Created by TD on 2023/1/13.
//
//
// Created by TD on 2022/8/26.
//

#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_core/database_task/details/tool.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/test/unit_test.hpp>

#include "sqlpp11/all_of.h"
#include "sqlpp11/select.h"
#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

using namespace doodle;
using namespace doodle::database_n;
BOOST_AUTO_TEST_CASE(test_sqlite3_create_table) {
  doodle_lib l_lib{};
  auto l_sql_conn = doodle_lib::Get().ctx().emplace<database_info>().get_connection();
  tables::work_task_info l_tables;
  l_sql_conn->execute(detail::create_table(l_tables).foreign_column(l_tables.entity_id, tables::entity{}.id).end());
  l_sql_conn->execute(detail::create_index(l_tables.entity_id));
  l_sql_conn->execute(detail::create_index(l_tables.id));
  (*l_sql_conn)(sqlpp::select(sqlpp::all_of(l_tables)).from(l_tables).unconditionally());

  (*l_sql_conn)(sqlpp::sqlite3::drop_if_exists_table(l_tables));
}
