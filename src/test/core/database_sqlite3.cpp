//
// Created by TD on 2023/1/13.
//
//
// Created by TD on 2022/8/26.
//

#include <doodle_core/core/core_sql.h>
#include <doodle_core/generate/core/sql_sql.h>
#include <doodle_core/pin_yin/convert.h>

#include <boost/test/unit_test.hpp>

#include <sqlpp11/sqlite3/sqlite3.h>
#include <sqlpp11/sqlpp11.h>

using namespace doodle;
namespace sql = doodle_database;
BOOST_AUTO_TEST_CASE(test_sqlite3_insert) {
  auto l_sql_conn = core_sql::Get().get_connection("D:/test.sqlite");

  l_sql_conn->execute(R"(
create table if not exists entity
(
    id          integer
        primary key,
    uuid_data   text,
    update_time datetime default CURRENT_TIMESTAMP not null
);
)"s);
  std::uint64_t l_r{};
  {
    auto l_t = sqlpp::start_transaction(*l_sql_conn);
    sql::Entity l_info{};
    auto l_pre =
        l_sql_conn->prepare(sqlpp::insert_into(l_info).set(l_info.uuidData = sqlpp::parameter(l_info.uuidData)));

    for (auto i = 0; i < 10; ++i) {
      l_pre.params.uuidData = fmt::format("tset {}", i);

      l_r                   = (*l_sql_conn)(l_pre);
      BOOST_TEST(l_r == (i + 1));
    }
    l_t.commit();
  }

  l_sql_conn.reset();
}
