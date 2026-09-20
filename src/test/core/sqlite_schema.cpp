//
// ORM 表结构 (schema) 断言.
// 这些用例只需要一个空库, 用于锁定 regs_all() 里声明的表结构, 不需要真实库.
//

#include <doodle_core/metadata/comment.h>
#include <doodle_lib/core/app_base.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <system_error>

BOOST_AUTO_TEST_SUITE(sqlite_schema)

namespace {

using namespace doodle;
using namespace doodle::orm;

FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_schema_{}.db", in_tag);
}

// 连同 -wal/-shm 一起删除; 用不抛异常的版本, 文件仍被连接占用时也不影响用例
void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"}) FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
}

std::string scalar_text(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
  return l_stmt.get_column_value<std::string>(0);
}

std::int64_t scalar_int(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
  return l_stmt.get_column_value<std::int64_t>(0);
}

std::string table_sql(orm::session& in_session, const std::string& in_table) {
  return scalar_text(in_session, fmt::format("SELECT sql FROM sqlite_master WHERE name = '{}';", in_table));
}

// 生成的 SQL 里被引用表名可能带引号也可能不带 (真实库中由旧代码建的表带引号,
// 当前 to_sql 生成的表名不带引号), 比较前先统一去掉引号
std::string strip_quotes(std::string in_sql) {
  std::erase(in_sql, '"');
  return in_sql;
}

}  // namespace

// comment.object_id 必须指向 task(uuid), 而不是 entity(uuid).
// 真实数据中 comment.object_type 全部是 "Task", 且没有一行 object_id 能匹配 entity.uuid,
// 原先指向 entity 的声明对全部数据都不成立 (会凭空产生 53 万条外键违规).
BOOST_AUTO_TEST_CASE(comment_object_id_references_task) {
  app_base l_app{};
  auto l_db = temp_db("comment");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.create_table<comment>();

    auto l_raw = table_sql(l_session, "comment");
    BOOST_TEST_MESSAGE(fmt::format("comment 表外键: {}", l_raw.substr(l_raw.rfind("FOREIGN KEY"))));
    auto l_sql = strip_quotes(l_raw);
    BOOST_TEST(l_sql.find("REFERENCES task(uuid)") != std::string::npos);
    BOOST_TEST(l_sql.find("REFERENCES entity(uuid)") == std::string::npos);
  }

  remove_db(l_db);
}

// 改过外键之后, 整套 regs_all() 表结构仍然可以在空库上完整建立
// (add_foreign_key 会顺带为被引用列生成索引, 建索引必须发生在建表之后)
BOOST_AUTO_TEST_CASE(sync_schema_creates_all_tables) {
  app_base l_app{};
  auto l_db = temp_db("sync");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.sync_schema();

    auto l_table_count = scalar_int(l_session, "SELECT count(*) FROM sqlite_master WHERE type = 'table';");
    BOOST_TEST_MESSAGE(fmt::format("sync_schema 建立了 {} 张表", l_table_count));
    BOOST_TEST(l_table_count > 50);
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
    // 空库上应当没有任何外键违规
    BOOST_TEST(l_session.pragma().foreign_key_check().empty());
  }

  remove_db(l_db);
}

BOOST_AUTO_TEST_SUITE_END()
