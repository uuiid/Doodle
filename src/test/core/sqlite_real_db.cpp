//
// 真实数据库端到端测试.
//
// 通过环境变量 DOODLE_REAL_DB 指定真实库路径:
//     $env:DOODLE_REAL_DB = "E:\Doodle\build\kitsu_new.db"
//     test_main.exe --run_test=sqlite_real_db
// 未设置时全部用例直接跳过, 不影响常规测试运行.
//
// 所有用例只操作真实库的**副本**, 绝不修改源库.
//

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <cstdlib>
#include <map>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(sqlite_real_db)

namespace {

using namespace doodle;
using namespace doodle::orm;

// 从环境变量取真实库路径; 未设置返回空路径
FSys::path real_db_path() {
  const char* l_env = std::getenv("DOODLE_REAL_DB");
  if (l_env == nullptr || *l_env == '\0') return {};
  return FSys::path{l_env};
}

// 连同 -wal/-shm 一起删除; 用不抛异常的版本, 文件仍被连接占用时也不影响用例
void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"}) FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
}

// 把真实库复制到临时目录. 必须连同 -wal/-shm 一起复制, 否则 WAL 里尚未 checkpoint 的数据会丢.
FSys::path copy_real_db(const FSys::path& in_src, const std::string& in_tag) {
  auto l_dst = FSys::temp_directory_path() / fmt::format("doodle_test_real_{}.db", in_tag);
  remove_db(l_dst);
  FSys::copy_file(in_src, l_dst);
  for (const auto* l_suffix : {"-wal", "-shm"}) {
    auto l_side = FSys::path{in_src.generic_string() + l_suffix};
    if (FSys::exists(l_side)) FSys::copy_file(l_side, FSys::path{l_dst.generic_string() + l_suffix});
  }
  return l_dst;
}

std::int64_t scalar_int(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
  return l_stmt.get_column_value<std::int64_t>(0);
}

std::string scalar_text(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
  return l_stmt.get_column_value<std::string>(0);
}

// 以 _backup 结尾的残留中间表数量
std::int64_t backup_table_count(orm::session& in_session) {
  return scalar_int(in_session, "SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name GLOB '*_backup'");
}

// 按 "子表 -> 父表" 汇总外键违规
std::map<std::string, std::int64_t> fk_by_child_parent(orm::session& in_session) {
  std::map<std::string, std::int64_t> l_result{};
  for (const auto& l_e : in_session.pragma().foreign_key_check())
    ++l_result[fmt::format("{} -> {}", l_e.table, l_e.parent)];
  return l_result;
}

std::int64_t fk_total(const std::map<std::string, std::int64_t>& in_map) {
  std::int64_t l_result{};
  for (const auto& [l_key, l_value] : in_map) l_result += l_value;
  return l_result;
}

// 库中所有表的行数
std::map<std::string, std::int64_t> all_table_counts(orm::session& in_session) {
  std::map<std::string, std::int64_t> l_result{};
  sqlite_stmt l_stmt{in_session, "SELECT name FROM sqlite_master WHERE type = 'table' ORDER BY name"};
  while (l_stmt.step_not_throw() == SQLITE_ROW) {
    auto l_name      = l_stmt.get_column_value<std::string>(0);
    l_result[l_name] = scalar_int(in_session, fmt::format(R"(SELECT count(*) FROM "{}";)", l_name));
  }
  return l_result;
}

// 比对两次行数快照, 返回发生变化的业务表描述 (sqlite_ 开头的是 SQLite 自己维护的内部表, 跳过)
std::vector<std::string> diff_business_counts(
    const std::map<std::string, std::int64_t>& in_before, const std::map<std::string, std::int64_t>& in_after
) {
  std::vector<std::string> l_result{};
  for (const auto& [l_name, l_value] : in_before) {
    if (l_name.starts_with("sqlite_")) continue;
    auto l_now = in_after.contains(l_name) ? in_after.at(l_name) : std::int64_t{-1};
    if (l_now != l_value) l_result.push_back(fmt::format("{} {} -> {}", l_name, l_value, l_now));
  }
  return l_result;
}

// 未设置环境变量时打印提示并跳过
bool skip_if_no_real_db(const FSys::path& in_path) {
  if (!in_path.empty()) return false;
  BOOST_TEST_MESSAGE("未设置 DOODLE_REAL_DB, 跳过真实库测试");
  return true;
}

}  // namespace

// 真实库上重建全部表: 数据保留、FTS 索引不被写重复、无中间表残留、结构完好、可重复执行
BOOST_AUTO_TEST_CASE(rebuild_all_tables_on_real_db) {
  auto l_src = real_db_path();
  if (skip_if_no_real_db(l_src)) return;
  BOOST_REQUIRE(FSys::exists(l_src));
  auto l_db = copy_real_db(l_src, "rebuild");

  app_base l_app{};
  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();

    auto l_entity_before  = scalar_int(l_session, "SELECT count(*) FROM entity");
    auto l_task_before    = scalar_int(l_session, "SELECT count(*) FROM task");
    auto l_docsize_before = scalar_int(l_session, "SELECT count(*) FROM entity_fts_docsize");
    auto l_counts_before  = all_table_counts(l_session);
    auto l_fk_before      = fk_by_child_parent(l_session);
    BOOST_TEST_MESSAGE(fmt::format(
        "重建前: entity={} task={} entity_fts_docsize={} 表数={} 外键违规={}", l_entity_before, l_task_before,
        l_docsize_before, l_counts_before.size(), fk_total(l_fk_before)
    ));

    auto l_count = l_storage.rebuild_all_tables(l_session);
    BOOST_TEST_MESSAGE(fmt::format("共重建 {} 张表", l_count));
    BOOST_TEST(l_count > 0);

    // 1. 业务表数据完整保留, 且没有表消失
    auto l_counts_after = all_table_counts(l_session);
    auto l_changed      = diff_business_counts(l_counts_before, l_counts_after);
    for (const auto& l_msg : l_changed) BOOST_TEST_MESSAGE(fmt::format("行数变化: {}", l_msg));
    BOOST_TEST(l_changed.empty());
    BOOST_TEST(l_counts_after.size() == l_counts_before.size());

    // 关键大表单独确认
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM entity") == l_entity_before);
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM task") == l_task_before);
    // FTS 索引与内容表同步: 复制数据时没有重复触发 entity 的 FTS 同步触发器
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM entity_fts_docsize") == l_docsize_before);

    // 2. 除已知 schema 差异外, 不新增外键违规来源.
    //    rebuild_all_tables 是按**当前代码的 ORM schema** 重建表的, 所以代码里比库新的外键会被落库.
    //    已知差异: 代码里 comment.object_id 被声明为 REFERENCES entity(uuid), 但真实数据中
    //    object_id 是配合 object_type 的多态引用列 (object_type 全部为 'Task'), 没有一行能匹配 entity.uuid.
    auto l_fk_after = fk_by_child_parent(l_session);
    for (const auto& [l_key, l_value] : l_fk_after) {
      auto l_was = l_fk_before.contains(l_key) ? l_fk_before.at(l_key) : std::int64_t{0};
      if (l_key == "comment -> entity") {
        BOOST_TEST_MESSAGE(fmt::format("已知 schema 差异 (非本次回归): {} {} -> {}", l_key, l_was, l_value));
        continue;
      }
      BOOST_TEST_MESSAGE(fmt::format("外键违规变化: {} {} -> {}", l_key, l_was, l_value));
      BOOST_TEST(l_value <= l_was);
    }
    BOOST_TEST_MESSAGE(fmt::format("重建后外键违规 = {}", fk_total(l_fk_after)));

    // 3. 没有 _backup 中间表残留
    BOOST_TEST(backup_table_count(l_session) == 0);

    // 4. 表结构完好
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");

    // 5. 幂等: 再重建一次, 行数与违规都不再变化
    BOOST_TEST(l_storage.rebuild_all_tables(l_session) == l_count);
    auto l_counts_second = all_table_counts(l_session);
    auto l_changed2      = diff_business_counts(l_counts_after, l_counts_second);
    for (const auto& l_msg : l_changed2) BOOST_TEST_MESSAGE(fmt::format("二次重建行数变化: {}", l_msg));
    BOOST_TEST(l_changed2.empty());
    BOOST_TEST(fk_by_child_parent(l_session) == l_fk_after);
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
  }

  remove_db(l_db);
}

// 真实库上 VACUUM INTO: 导出副本可用, 行数与外键状态一致, 结构完好
BOOST_AUTO_TEST_CASE(vacuum_into_on_real_db) {
  auto l_src = real_db_path();
  if (skip_if_no_real_db(l_src)) return;
  BOOST_REQUIRE(FSys::exists(l_src));
  auto l_db  = copy_real_db(l_src, "vacuum_src");
  auto l_out = FSys::temp_directory_path() / "doodle_test_real_vacuum_out.db";
  remove_db(l_out);

  app_base l_app{};
  std::int64_t l_entity_before{0};
  std::int64_t l_fk_before{0};

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session  = l_storage.create_session();
    l_entity_before = scalar_int(l_session, "SELECT count(*) FROM entity");
    l_fk_before     = static_cast<std::int64_t>(l_session.pragma().foreign_key_check().size());

    l_session.pragma().vacuum_into(l_out);
    BOOST_REQUIRE(FSys::exists(l_out));
  }

  {
    sqlite_storage l_out_storage{};
    l_out_storage.open(l_out);
    auto l_session = l_out_storage.create_session();
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM entity") == l_entity_before);
    BOOST_TEST(static_cast<std::int64_t>(l_session.pragma().foreign_key_check().size()) == l_fk_before);
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
    BOOST_TEST_MESSAGE(fmt::format(
        "源副本 {} 字节 -> VACUUM INTO 导出 {} 字节", FSys::file_size(l_db), FSys::file_size(l_out)
    ));
  }

  remove_db(l_out);
  remove_db(l_db);
}

BOOST_AUTO_TEST_SUITE_END()
