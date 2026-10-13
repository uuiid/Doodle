//
// 数据库版本升级测试.
//
// 升级器的结构是"一步一个版本": upgrade_N_t 各自判断自己该不该跑.
// 本轮之前链条是 29 -> 30 -> 31; 29 -> 30 已随该步骤进入生产而移除 (见 sqlite_upgrade.cpp 顶部注释),
// 所以现在**只剩 30 -> 31 一步** (生产库停在 30), 升级链也只认 30 及以上的库.
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

// 表上是否已有某列
bool has_column(orm::session& in_session, const std::string& in_table, const std::string& in_column) {
  return scalar_int(
             in_session,
             fmt::format("SELECT count(*) FROM pragma_table_info('{}') WHERE name = '{}';", in_table, in_column)
         ) > 0;
}

// 表是否存在
bool has_table(orm::session& in_session, const std::string& in_table) {
  return scalar_int(
             in_session,
             fmt::format("SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name = '{}';", in_table)
         ) > 0;
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

// 把 seedance2_task_2 退回到**真正的 v30 形状**（即缺少 v31 才补的两列）。
//
// 为什么必须做这一步：`sync_schema()` 是按**最新**声明建表的，而最新声明里已经有
// ai_light_iteration_id / result_text（sqlite_database.cpp:118-119）。所以刚 `sync_schema()`
// 出来的库虽然写着 user_version=26，表的形状其实是 v31 —— 不是我们要模拟的那个旧库。
//
// 不处理的后果是硬的：upgrade_3_t 第一步就是
// `ALTER TABLE "seedance2_task_2" ADD COLUMN "ai_light_iteration_id" BLOB;`
//（sqlite_upgrade.cpp:155，session::add_column 是裸 ALTER，没有「列已存在就跳过」的判断），
// 撞上已存在的列直接报 `duplicate column name: ai_light_iteration_id`，升级整个中断。
// 这是**夹具自己造的**假象：真实库不会在同一张表上重复加列（迁移由 user_version 门控只跑一次）。
//
// 注意不能用 create_table<seedance2::task>() 重建 —— 它按 ORM 声明建表，声明里有这两列，
// 建出来还是 v31 形状等于没退。也不能 ALTER TABLE ... DROP COLUMN —— ai_light_iteration_id
// 被外键定义引用着，SQLite 会拒绝（`unknown column ... in foreign key definition`）。
// 所以手写一份**只含 v30 列**的建表语句：列清单照抄 sqlite_database.cpp:99-121，
// 去掉 ai_light_iteration_id / result_text；外键一并去掉，免得引用尚未建出的 ai_light_iteration。
// 与 ai_light_schema.cpp 里 rebuild_task_table_without_v31_columns() 是同一份口径。
void rebuild_task_table_without_v31_columns(orm::session& in_session) {
  const auto l_fk_was_on = in_session.pragma().foreign_keys();
  in_session.pragma().foreign_keys(false);
  boost::scope::scope_exit l_fk_guard(
      [&in_session, l_fk_was_on]() { in_session.pragma().foreign_keys(l_fk_was_on); }
  );
  exec_sql(in_session, "DROP TABLE IF EXISTS seedance2_task_2;");
  exec_sql(
      in_session,
      R"(CREATE TABLE seedance2_task_2 (
           id INTEGER PRIMARY KEY AUTOINCREMENT,
           uuid_id TEXT NOT NULL UNIQUE,
           user_id TEXT,
           status INTEGER,
           type INTEGER NOT NULL,
           backend INTEGER NOT NULL,
           data_request TEXT,
           text_prompt TEXT,
           preview_file TEXT,
           file_extension TEXT,
           data_response TEXT,
           ai_studio_id TEXT,
           task_id TEXT,
           created_at TEXT,
           ended_at TEXT,
           project_uuid_id TEXT,
           completion_tokens INTEGER,
           ai_generate_entity_id TEXT,
           subproject_id TEXT,
           retry_count INTEGER,
           archived INTEGER
         );)"
  );
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
    BOOST_TEST(l_version == 31);

    // 新建的库直接就是最新 schema: te_xie 列由 regs_all() 的声明建出来, 不需要升级补
    BOOST_TEST(has_column(l_session, "entity_asset_extend_2", "te_xie"));

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

// 比 30 更旧的库也会被剩下的那一步带到最新: 不能因为"版本不等于 30"就既不升级、也不写版本号,
// 那样它会永远停在旧状态上.
//
// 本用例是 `> N 就跳过` 那个写法的守门人: 把 upgrade_3_t 的门控从 `> 30` 改成 `== 30`,
// 这个从 v26 出发的库就会既不建表也不写版本号, 在这里失败.
//
// 注意这里**只**断言版本号与整体健康: 29 -> 30 (给 entity_asset_extend_2 补 te_xie 列) 已随该步骤
// 进入生产而移除, 所以 v26 的库跑完也拿不到 te_xie 列 —— 升级链现在只认 30 及以上,
// 见 sqlite_upgrade.cpp 顶部版本号注释.
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

    // sync_schema() 建的是**最新**形状，而我们要的是 v26 的旧库：先把 seedance2_task_2
    // 退回缺少 v31 两列的形状，否则 upgrade_3_t 的 add_column 会撞上已有列报
    // duplicate column name（详见该函数上方的注释）。
    rebuild_task_table_without_v31_columns(l_session);

    l_session.pragma().user_version(26);
    l_storage.upgrade();
    auto l_version = scalar_int(l_session, "PRAGMA user_version;");
    BOOST_TEST_MESSAGE(fmt::format("v26 升级后 user_version = {}", l_version));
    BOOST_TEST(l_version == 31);
    // 剩下的那一步的副作用确实存在, 说明它没有被跳过
    BOOST_TEST(has_table(l_session, "ai_light_session"));
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
