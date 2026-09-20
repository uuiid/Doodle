//
// 数据库版本升级测试.
//
// 升级器的结构是"一步一个版本": upgrade_N_t 负责把库从 N-1 升到 N, 各自判断自己该不该跑.
// 这种结构下最危险的错误是"跳过"—— 某个步骤没执行, 但 user_version 被写成了最新版,
// 于是库看起来是新的, 实际迁移根本没做. 这里的用例专门盯住这一点.
//

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/scope/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <set>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(sqlite_upgrade)

namespace {

using namespace doodle;
using namespace doodle::orm;

FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_upgrade_{}.db", in_tag);
}

void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"}) FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
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

void exec_sql(orm::session& in_session, const std::string& in_sql) {
  sqlite_stmt l_stmt{in_session, in_sql};
  l_stmt.step();
}

// 纯冗余索引名: 显式索引的列集合与同表某个自动索引 (UNIQUE/PK 生成) 完全相同.
// 返回名字而不是数量, 失败时能直接看出是哪一个.
std::vector<std::string> redundant_index_names(orm::session& in_session) {
  std::vector<std::string> l_result{};
  sqlite_stmt l_stmt{
      in_session,
      R"(SELECT m.name FROM sqlite_master m
         WHERE m.type = 'index' AND m.sql IS NOT NULL
           AND EXISTS (
             SELECT 1 FROM sqlite_master a
             WHERE a.type = 'index' AND a.sql IS NULL AND a.tbl_name = m.tbl_name
               AND (SELECT group_concat(ii.name) FROM pragma_index_info(a.name) ii)
                 = (SELECT group_concat(ii.name) FROM pragma_index_info(m.name) ii)
           );)"
  };
  while (l_stmt.step_not_throw() == SQLITE_ROW) l_result.push_back(l_stmt.get_column_value<std::string>(0));
  return l_result;
}

// 备份目录里的文件名集合
std::set<std::string> list_backup_files(const FSys::path& in_dir) {
  std::set<std::string> l_result{};
  std::error_code l_ec{};
  if (!FSys::exists(in_dir, l_ec)) return l_result;
  for (const auto& l_entry : FSys::directory_iterator{in_dir, l_ec}) {
    if (l_entry.is_regular_file(l_ec)) l_result.insert(l_entry.path().filename().string());
  }
  return l_result;
}

// 升级会往 cache 目录写一份完整备份. 用例结束时只删掉**本次新建**的那几个,
// 不碰用户已有的备份 —— 否则每跑一次测试就在用户缓存里留一份垃圾.
class backup_cleaner {
  FSys::path dir_;
  std::set<std::string> before_;

 public:
  backup_cleaner() : dir_(core_set::get_set().get_cache_root("backup")), before_(list_backup_files(dir_)) {}
  backup_cleaner(const backup_cleaner&)            = delete;
  backup_cleaner& operator=(const backup_cleaner&) = delete;
  ~backup_cleaner() {
    std::error_code l_ec{};
    for (const auto& l_entry : FSys::directory_iterator{dir_, l_ec}) {
      if (!l_entry.is_regular_file(l_ec)) continue;
      if (before_.contains(l_entry.path().filename().string())) continue;
      FSys::remove(l_entry.path(), l_ec);
    }
  }
};

// 打印冗余索引并在存在时返回其数量
std::size_t report_redundant_indexes(orm::session& in_session, const std::string& in_where) {
  auto l_names = redundant_index_names(in_session);
  for (const auto& l_name : l_names) BOOST_TEST_MESSAGE(fmt::format("{} 的冗余索引: {}", in_where, l_name));
  return l_names.size();
}

}  // namespace

// 全新库 (user_version == 0) 的初始化路径必须在外键开启的情况下走通.
// upgrade_init_t 会 sync_schema() 建全部表并插入内置常量行 (project_status / asset_type /
// task_type / assets), 这条路径此前只在外键默认关闭时跑过, 是开启外键后最需要回归的一环.
// 关键前提: 可空引用列未赋值时是 nil uuid, 由 sqlite_statement.h 绑定成 NULL, 而 NULL 永远
// 满足外键约束 —— 所以未赋值的可选引用不会误报违规.
BOOST_AUTO_TEST_CASE(fresh_db_upgrade_succeeds_with_fk_enforced) {
  app_base l_app{};
  backup_cleaner l_cleaner{};
  auto l_db = temp_db("fresh");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    l_storage.upgrade();

    auto l_session = l_storage.create_session();
    auto l_version = scalar_int(l_session, "PRAGMA user_version;");
    BOOST_TEST_MESSAGE(fmt::format("全新库 user_version = {}", l_version));
    BOOST_TEST(l_version > 0);

    auto l_task_type_count = scalar_int(l_session, "SELECT count(*) FROM task_type;");
    BOOST_TEST_MESSAGE(fmt::format("内置 task_type 常量行数 = {}", l_task_type_count));
    BOOST_TEST(l_task_type_count >= 11);

    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
    // 全新库插入完常量后不应产生任何外键违规
    auto l_bad = l_session.pragma().foreign_key_check();
    for (const auto& l_entry : l_bad) {
      BOOST_TEST_MESSAGE(fmt::format("全新库外键违规: {} rowid={} -> {}", l_entry.table, l_entry.rowid, l_entry.parent));
    }
    BOOST_TEST(l_bad.empty());
  }

  remove_db(l_db);
}

// 升级步骤必须真的执行, 且不能对已经是最新版的库重复执行.
//
// 这类错误不会有任何报错: 只要 user_version 被写成了最新版, 库"看起来"就是新的, 迁移做没做
// 从版本号上完全看不出来. 只能靠断言把步骤的**副作用**盯住.
//
// 探针用一张**未注册**的表: rebuild_all_tables 只重建 regs_all() 里注册过的表, 碰不到它,
// 所以它上面冗余索引的消失只可能来自升级步骤调用的 drop_redundant_indexes.
BOOST_AUTO_TEST_CASE(upgrade_advances_one_version_at_a_time) {
  app_base l_app{};
  backup_cleaner l_cleaner{};
  auto l_db = temp_db("gating");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.sync_schema();

    // P1-1: sync_schema 不应再生成与被引用列 UNIQUE 自动索引重复的索引
    // (历史上 add_foreign_key 会为被引用列也建一个索引, 加上 5 处显式的 uuid 索引, 共累积出 40 个)
    BOOST_TEST(report_redundant_indexes(l_session, "sync_schema 后") == 0);

    // 未注册的遗留表: u 上已有 UNIQUE 自动索引, 再显式建一个同列索引即为纯冗余
    exec_sql(l_session, "CREATE TABLE legacy_probe(id INTEGER PRIMARY KEY, u TEXT UNIQUE);");
    exec_sql(l_session, "CREATE INDEX idx_legacy_probe_u ON legacy_probe(u);");

    // 1. 生产库当前的版本: 升级步骤必须执行
    l_session.pragma().user_version(27);
    l_storage.upgrade();
    auto l_version = scalar_int(l_session, "PRAGMA user_version;");
    BOOST_TEST_MESSAGE(fmt::format("v27 升级后 user_version = {}", l_version));
    BOOST_TEST(l_version == 28);
    BOOST_TEST(report_redundant_indexes(l_session, "v27 升级后") == 0);  // 证明升级步骤真的跑了

    // 2. 已是最新版的库: 不应该有任何步骤执行
    exec_sql(l_session, "CREATE INDEX idx_legacy_probe_u2 ON legacy_probe(u);");
    l_session.pragma().user_version(28);
    l_storage.upgrade();
    BOOST_TEST(scalar_int(l_session, "PRAGMA user_version;") == 28);
    BOOST_TEST(report_redundant_indexes(l_session, "v28 升级后") == 1);  // 原样保留, 证明没有重复执行
  }

  remove_db(l_db);
}

// 比 27 更旧的库也要被这一步带到最新: 不能因为"版本不等于 27"就既不升级、也不写版本号,
// 那样它会永远停在旧 schema 上 (见 upgrade_1_t 里关于 `> 27 就跳过` 的说明)
BOOST_AUTO_TEST_CASE(upgrade_brings_older_db_up_to_current) {
  app_base l_app{};
  backup_cleaner l_cleaner{};
  auto l_db = temp_db("from26");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.sync_schema();

    exec_sql(l_session, "CREATE TABLE legacy_probe(id INTEGER PRIMARY KEY, u TEXT UNIQUE);");
    exec_sql(l_session, "CREATE INDEX idx_legacy_probe_u ON legacy_probe(u);");

    l_session.pragma().user_version(26);
    l_storage.upgrade();
    auto l_version = scalar_int(l_session, "PRAGMA user_version;");
    BOOST_TEST_MESSAGE(fmt::format("v26 升级后 user_version = {}", l_version));
    BOOST_TEST(l_version == 28);
    // 升级步骤的副作用确实存在, 说明它没有被跳过
    BOOST_TEST(report_redundant_indexes(l_session, "v26 升级后") == 0);
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
    BOOST_TEST(l_session.pragma().foreign_key_check().empty());
  }

  remove_db(l_db);
}

// null_dangling_optional_references 必须把悬空引用**置空**, 而不是删行.
//
// 这是它与 fix_foreign_key_violations 的关键区别, 也是本次最容易搞错的地方:
// 后者处理的行本身就不该存在 (孤儿子行), 而 task.last_preview_file_id 这类可空可选归属列上的行
// 是**有效任务**, 只是指向了已经删除的预览文件. 交给 fix_foreign_key_violations 处理就会把
// 79 个任务整行删掉 —— 真实库上正是这个数量.
BOOST_AUTO_TEST_CASE(dangling_optional_references_are_nulled_not_deleted) {
  app_base l_app{};
  backup_cleaner l_cleaner{};
  auto l_db = temp_db("dangling");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    l_storage.upgrade();

    auto l_session = l_storage.create_session();

    // 造数据时必须关外键: 悬空引用正是要模拟的非法状态, 开着外键根本插不进去.
    // 关闭状态不能泄漏, 用 guard 恢复.
    {
      const auto l_fk_was_on = l_session.pragma().foreign_keys();
      l_session.pragma().foreign_keys(false);
      boost::scope::scope_exit l_fk_guard(
          [&l_session, l_fk_was_on]() { l_session.pragma().foreign_keys(l_fk_was_on); }
      );

      // 一个真实存在的预览文件, 用来构造"有效引用"
      exec_sql(l_session, R"(
        INSERT INTO preview_file (uuid, revision, position, source, file_size, status, validation_status,
                                  width, height, duration, shotgun_id, is_movie, created_at, updated_at)
        VALUES (x'000000000000000000000000000000A1', 0, 0, '', 0, '', '', 0, 0, 0.0, 0, 0, '', '');)");
      // 两个任务: 一个指向存在的预览文件 (有效), 一个指向不存在的 (悬空)
      exec_sql(l_session, R"(
        INSERT INTO task (uuid, priority, difficulty, duration, estimation, completion_rate, retake_count,
                          sort_order, nb_assets_ready, shotgun_id, nb_drawings, created_at, updated_at,
                          last_preview_file_id)
        VALUES (x'000000000000000000000000000000B1', 0, 0, 0.0, 0.0, 0, 0, 0, 0, 0, 0, '', '',
                x'000000000000000000000000000000A1');)");
      exec_sql(l_session, R"(
        INSERT INTO task (uuid, priority, difficulty, duration, estimation, completion_rate, retake_count,
                          sort_order, nb_assets_ready, shotgun_id, nb_drawings, created_at, updated_at,
                          last_preview_file_id)
        VALUES (x'000000000000000000000000000000B2', 0, 0, 0.0, 0.0, 0, 0, 0, 0, 0, 0, '', '',
                x'000000000000000000000000000000FF');)");
      // 工时记录: 一条指向存在的任务 (有效), 一条指向不存在的 (悬空)
      exec_sql(l_session, R"(
        INSERT INTO work_xlsx_task_info_tab (uuid_id, start_time, end_time, duration, year_month, kitsu_task_ref_id)
        VALUES (x'000000000000000000000000000000C1', '', '', 0, 202601, x'000000000000000000000000000000B1');)");
      exec_sql(l_session, R"(
        INSERT INTO work_xlsx_task_info_tab (uuid_id, start_time, end_time, duration, year_month, kitsu_task_ref_id)
        VALUES (x'000000000000000000000000000000C2', '', '', 0, 202601, x'000000000000000000000000000000FF');)");
    }

    // 前置: 两处悬空确实存在, 外键检查能看到
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM task;") == 2);
    BOOST_TEST(!l_session.pragma().foreign_key_check().empty());

    auto l_nulled = l_storage.null_dangling_optional_references(l_session);
    BOOST_TEST_MESSAGE(fmt::format("置空 {} 行悬空引用", l_nulled));
    BOOST_TEST(l_nulled == 2);

    // 关键: 行都还在, 只是引用被置空
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM task;") == 2);
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM work_xlsx_task_info_tab;") == 2);
    BOOST_TEST(
        scalar_int(l_session, "SELECT count(*) FROM task WHERE uuid = x'000000000000000000000000000000B2';") == 1
    );
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM task WHERE last_preview_file_id IS NULL;") == 1);
    BOOST_TEST(
        scalar_int(l_session, "SELECT count(*) FROM work_xlsx_task_info_tab WHERE kitsu_task_ref_id IS NULL;") == 1
    );
    // 有效引用不能被误伤
    BOOST_TEST(
        scalar_int(
            l_session, "SELECT count(*) FROM task WHERE last_preview_file_id = x'000000000000000000000000000000A1';"
        ) == 1
    );
    BOOST_TEST(
        scalar_int(
            l_session,
            "SELECT count(*) FROM work_xlsx_task_info_tab WHERE kitsu_task_ref_id = "
            "x'000000000000000000000000000000B1';"
        ) == 1
    );
    // 置空之后外键检查必须干净
    BOOST_TEST(l_session.pragma().foreign_key_check().empty());

    // 幂等: 再跑一次不应该再改任何东西
    BOOST_TEST(l_storage.null_dangling_optional_references(l_session) == 0);
  }

  remove_db(l_db);
}

BOOST_AUTO_TEST_SUITE_END()
