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
#include <vector>

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

// 某表上"列集合恰为 in_columns (顺序即索引内列序) 且唯一性等于 in_unique"的索引数量.
// 全部用常量参数调用 pragma_* 表值函数, 避免相关子查询形式在不同 SQLite 版本上的差异.
std::int64_t count_indexes(
    orm::session& in_session, const std::string& in_table, const std::string& in_columns, bool in_unique
) {
  std::int64_t l_count{0};
  sqlite_stmt l_list{in_session, fmt::format(R"(SELECT name, "unique" FROM pragma_index_list('{}');)", in_table)};
  while (l_list.step_not_throw() == SQLITE_ROW) {
    auto l_name = l_list.get_column_value<std::string>(0);
    if ((l_list.get_column_value<std::int32_t>(1) != 0) != in_unique) continue;
    sqlite_stmt l_info{in_session, fmt::format("SELECT name FROM pragma_index_info('{}');", l_name)};
    std::vector<std::string> l_cols{};
    while (l_info.step_not_throw() == SQLITE_ROW) l_cols.push_back(l_info.get_column_value<std::string>(0));
    if (fmt::format("{}", fmt::join(l_cols, ",")) == in_columns) ++l_count;
  }
  return l_count;
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

// 本次 schema 清理的结果: 废弃的表/列不再创建, 唯一性约束的形态正确.
// 老库里的废弃表由升级步骤的 drop_obsolete_tables 删除 (rebuild_all_tables 只处理已注册的表).
BOOST_AUTO_TEST_CASE(schema_cleanup_shape_is_correct) {
  app_base l_app{};
  auto l_db = temp_db("cleanup");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.sync_schema();

    // 1. 已废弃的表不再创建
    for (const auto* l_name : {"metadata_descriptor", "metadata_descriptor_department_link", "ai_image_metadata"}) {
      auto l_exists = scalar_int(
          l_session, fmt::format("SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name = '{}';", l_name)
      );
      BOOST_TEST_MESSAGE(fmt::format("废弃表 {} 是否仍存在: {}", l_name, l_exists));
      BOOST_TEST(l_exists == 0);
    }

    // 2. preview_file.source_file_id 已废弃但**保留声明**: 重建时列的拷贝清单来自 ORM 声明,
    //    一旦摘掉这一列就会在本次重建中被真正删除, 那属于另一次变更
    auto l_source_file_id = scalar_int(
        l_session, "SELECT count(*) FROM pragma_table_info('preview_file') WHERE name = 'source_file_id';"
    );
    BOOST_TEST_MESSAGE(fmt::format("preview_file.source_file_id 列数 = {}", l_source_file_id));
    BOOST_TEST(l_source_file_id == 1);

    // 3. preview_file.name 不能有单列唯一约束 (同一 name 在不同 task/revision 下合法重复),
    //    唯一性由 (name, task_id, revision) 复合唯一索引保证
    BOOST_TEST(count_indexes(l_session, "preview_file", "name", true) == 0);
    auto l_composite = count_indexes(l_session, "preview_file", "name,task_id,revision", true);
    BOOST_TEST_MESSAGE(fmt::format("preview_file (name,task_id,revision) 唯一索引数 = {}", l_composite));
    BOOST_TEST(l_composite == 1);

    // 4. 两个链接表必须有唯一索引, 否则同一对 (项目, X) 可以重复插入
    struct link_t {
      const char* table_;
      const char* columns_;
    };
    for (const auto& l_link : {link_t{"project_asset_type_link", "project_id,asset_type_id"},
                               link_t{"project_person_link", "project_id,person_id"}}) {
      auto l_unique = count_indexes(l_session, l_link.table_, l_link.columns_, true);
      BOOST_TEST_MESSAGE(fmt::format("{}({}) 唯一索引数 = {}", l_link.table_, l_link.columns_, l_unique));
      BOOST_TEST(l_unique == 1);
    }
  }

  remove_db(l_db);
}

BOOST_AUTO_TEST_SUITE_END()
