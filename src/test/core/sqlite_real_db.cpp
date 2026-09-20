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
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/test/unit_test.hpp>

#include <boost/scope/scope_exit.hpp>

#include <cstdint>
#include <cstdlib>
#include <map>
#include <set>
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

// 生成的 SQL 里被引用表名可能带引号也可能不带 (真实库中由旧代码建的表带引号,
// 当前 to_sql 生成的表名不带引号), 比较前先统一去掉引号
std::string strip_quotes(std::string in_sql) {
  std::erase(in_sql, '"');
  return in_sql;
}

// 索引总数 (含 sqlite_autoindex_*)
std::int64_t count_indexes(orm::session& in_session) {
  return scalar_int(in_session, "SELECT count(*) FROM sqlite_master WHERE type = 'index';");
}

// 纯冗余索引数量: 显式索引的列集合与同表某个自动索引 (UNIQUE/PK 生成) 完全相同.
// 若这个相关子查询形式不被支持会静默返回 0, 所以调用处都断言了期望值, 不会假通过.
std::int64_t count_redundant_indexes(orm::session& in_session) {
  return scalar_int(
      in_session,
      R"(SELECT count(*) FROM sqlite_master m
         WHERE m.type = 'index' AND m.sql IS NOT NULL
           AND EXISTS (
             SELECT 1 FROM sqlite_master a
             WHERE a.type = 'index' AND a.sql IS NULL AND a.tbl_name = m.tbl_name
               AND (SELECT group_concat(ii.name) FROM pragma_index_info(a.name) ii)
                 = (SELECT group_concat(ii.name) FROM pragma_index_info(m.name) ii)
           );)"
  );
}

// 备份目录里的文件名集合 (升级会往这里写一份完整备份)
std::set<std::string> list_backup_files(const FSys::path& in_dir) {
  std::set<std::string> l_result{};
  std::error_code l_ec{};
  if (!FSys::exists(in_dir, l_ec)) return l_result;
  for (const auto& l_entry : FSys::directory_iterator{in_dir, l_ec}) {
    if (l_entry.is_regular_file(l_ec)) l_result.insert(l_entry.path().filename().string());
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

    // 2. 外键: rebuild_all_tables 是按**当前代码的 ORM schema** 重建表的, 所以代码里比库新的外键会被落库.
    //    真实库的 comment 表还没有 object_id 外键, 重建会加上 comment.object_id -> task(uuid).
    //    因此重建后会暴露出"任务已被删除"的孤儿评论; 预期数量等于重建前直接查出来的孤儿数.
    auto l_orphan_comment = scalar_int(
        l_session, "SELECT count(*) FROM comment c WHERE NOT EXISTS (SELECT 1 FROM task t WHERE t.uuid = c.object_id);"
    );

    auto l_fk_after = fk_by_child_parent(l_session);

    // 修正后的外键确实落库, 且指向 task 而不再是 entity
    auto l_comment_raw = scalar_text(l_session, "SELECT sql FROM sqlite_master WHERE name = 'comment';");
    BOOST_TEST_MESSAGE(
        fmt::format("重建后 comment 表外键: {}", l_comment_raw.substr(l_comment_raw.rfind("FOREIGN KEY")))
    );
    auto l_comment_sql = strip_quotes(l_comment_raw);
    BOOST_TEST(l_comment_sql.find("REFERENCES task(uuid)") != std::string::npos);
    BOOST_TEST(l_comment_sql.find("REFERENCES entity(uuid)") == std::string::npos);
    BOOST_TEST(!l_fk_after.contains("comment -> entity"));

    for (const auto& [l_key, l_value] : l_fk_after) {
      auto l_was = l_fk_before.contains(l_key) ? l_fk_before.at(l_key) : std::int64_t{0};
      if (l_key == "comment -> task") {
        BOOST_TEST_MESSAGE(fmt::format(
            "新增外键暴露的孤儿评论: {} {} -> {} (重建前直接查询得到 {})", l_key, l_was, l_value, l_orphan_comment
        ));
        BOOST_TEST(l_value == l_orphan_comment);
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

// v28 迁移序列在真实库副本上的端到端验证: 重建全部表 -> 清理新暴露的孤儿 -> 清理冗余索引.
//
// 这里**不**调用 sqlite_storage::upgrade(): 它会先往 cache 目录写一份 ~750MB 的备份, 不适合放进
// 测试. 取而代之是按升级步骤的顺序执行同样三步, 验证组合之后的最终状态.
BOOST_AUTO_TEST_CASE(v28_migration_sequence_on_real_db) {
  auto l_src = real_db_path();
  if (skip_if_no_real_db(l_src)) return;
  BOOST_REQUIRE(FSys::exists(l_src));
  auto l_db = copy_real_db(l_src, "v28");

  app_base l_app{};
  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();

    auto l_counts_before    = all_table_counts(l_session);
    auto l_fk_before        = fk_total(fk_by_child_parent(l_session));
    auto l_comment_before   = scalar_int(l_session, "SELECT count(*) FROM comment");
    auto l_redundant_before = count_redundant_indexes(l_session);
    BOOST_TEST_MESSAGE(fmt::format(
        "迁移前: 外键违规={} 冗余索引={} comment={}", l_fk_before, l_redundant_before, l_comment_before
    ));
    BOOST_TEST(l_redundant_before > 0);

    // 1. 重建全部表, 落地新的外键目标/动作
    BOOST_TEST(l_storage.rebuild_all_tables(l_session) > 0);

    // 2. 清理重建后新暴露的孤儿子行 (与升级步骤一致: 关外键 + 事务)
    std::int64_t l_deleted{0};
    {
      const auto l_fk_was_on = l_session.pragma().foreign_keys();
      l_session.pragma().foreign_keys(false);
      boost::scope::scope_exit l_fk_guard([&l_session, l_fk_was_on]() {
        l_session.pragma().foreign_keys(l_fk_was_on);
      });
      auto l_tx  = l_session.transaction();
      l_deleted  = static_cast<std::int64_t>(l_storage.fix_foreign_key_violations(l_session));
      l_tx.commit();
    }
    BOOST_TEST_MESSAGE(fmt::format("清理外键孤儿子行 {} 行", l_deleted));
    BOOST_TEST(l_deleted > 0);

    // 3. 清理冗余索引
    auto l_indexes_before  = count_indexes(l_session);
    auto l_dropped         = static_cast<std::int64_t>(l_storage.drop_redundant_indexes(l_session));
    BOOST_TEST_MESSAGE(fmt::format("删除纯冗余索引 {} 个", l_dropped));
    BOOST_TEST(l_dropped > 0);
    // 只该少掉这些冗余索引, 正常索引一个都不能被误删
    BOOST_TEST_MESSAGE(fmt::format(
        "索引总数 {} -> {}", l_indexes_before, count_indexes(l_session)
    ));
    BOOST_TEST(count_indexes(l_session) == l_indexes_before - l_dropped);

    // 最终状态: 外键零违规、无冗余索引、无中间表残留、结构完好
    auto l_fk_after = fk_by_child_parent(l_session);
    for (const auto& [l_key, l_value] : l_fk_after)
      BOOST_TEST_MESSAGE(fmt::format("剩余外键违规: {} = {}", l_key, l_value));
    BOOST_TEST(l_fk_after.empty());
    BOOST_TEST(count_redundant_indexes(l_session) == 0);
    BOOST_TEST(backup_table_count(l_session) == 0);
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");

    // 业务数据只少了被清理的孤儿行, 没有别的损失
    auto l_comment_after = scalar_int(l_session, "SELECT count(*) FROM comment");
    BOOST_TEST_MESSAGE(fmt::format("comment: {} -> {}", l_comment_before, l_comment_after));
    BOOST_TEST(l_comment_after <= l_comment_before);
    BOOST_TEST(l_comment_before - l_comment_after <= l_deleted);

    auto l_changed = diff_business_counts(l_counts_before, all_table_counts(l_session));
    for (const auto& l_msg : l_changed) BOOST_TEST_MESSAGE(fmt::format("行数变化: {}", l_msg));
  }

  remove_db(l_db);
}

// 真实库上跑**真正的**升级路径 sqlite_storage::upgrade(), 即生产库从 27 升到 28 的那一条.
//
// v28_migration_sequence_on_real_db 只验证三个步骤本身; 这个用例验证包在外面的那一层:
// 版本判断、升级前的完整备份、以及 757MB 库上的 VACUUM (VACUUM 需要额外一份等大的临时空间,
// 是整条路径里最容易在真实环境上失败的一步). 升级会往 cache 目录写一份完整备份,
// 用例结束时只删掉**本次新建**的那一个, 不碰用户已有的备份.
BOOST_AUTO_TEST_CASE(upgrade_to_v28_on_real_db) {
  auto l_src = real_db_path();
  if (skip_if_no_real_db(l_src)) return;
  BOOST_REQUIRE(FSys::exists(l_src));
  auto l_db = copy_real_db(l_src, "upgrade");

  app_base l_app{};
  auto l_backup_dir    = core_set::get_set().get_cache_root("backup");
  auto l_backup_before = list_backup_files(l_backup_dir);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session   = l_storage.create_session();
    auto l_version   = scalar_int(l_session, "PRAGMA user_version;");
    auto l_fk_before = fk_total(fk_by_child_parent(l_session));
    auto l_comment   = scalar_int(l_session, "SELECT count(*) FROM comment");
    BOOST_TEST_MESSAGE(fmt::format(
        "升级前: user_version={} 外键违规={} 冗余索引={} comment={}", l_version, l_fk_before,
        count_redundant_indexes(l_session), l_comment
    ));
    // 这个用例的前提就是"生产库在 27"; 不满足说明拿到的是别的库, 断言出来而不是静默跑过
    BOOST_TEST(l_version == 27);

    l_storage.upgrade();

    auto l_after = scalar_int(l_session, "PRAGMA user_version;");
    BOOST_TEST_MESSAGE(fmt::format("升级后 user_version = {}", l_after));
    BOOST_TEST(l_after == 28);
    BOOST_TEST(scalar_text(l_session, "PRAGMA integrity_check;") == "ok");
    // 升级必须把外键违规清干净, 否则此后任何 UPDATE 到这些行都会失败
    auto l_fk_after = fk_by_child_parent(l_session);
    for (const auto& [l_key, l_value] : l_fk_after)
      BOOST_TEST_MESSAGE(fmt::format("剩余外键违规: {} = {}", l_key, l_value));
    BOOST_TEST(l_fk_after.empty());
    BOOST_TEST(count_redundant_indexes(l_session) == 0);
    BOOST_TEST(backup_table_count(l_session) == 0);
    // 废弃表必须被 drop_obsolete_tables 真正删掉: 它们不在 regs_all() 里, 重建碰不到
    for (const auto* l_name : {"metadata_descriptor", "metadata_descriptor_department_link", "ai_image_metadata"}) {
      auto l_exists = scalar_int(
          l_session, fmt::format("SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name = '{}';", l_name)
      );
      BOOST_TEST_MESSAGE(fmt::format("升级后废弃表 {} 是否仍存在: {}", l_name, l_exists));
      BOOST_TEST(l_exists == 0);
    }
    // 数据库是可用的: 大表还在, 只是少了被清理的孤儿行
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM comment") > 0);
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM task") > 0);

    // 升级后的整体结构. 外键总数 131 -> 133: 新增 task.last_preview_file_id 与
    // work_xlsx_task_info_tab.kitsu_task_ref_id 两个 set_null 外键 (其余 +3/-3 相互抵消, 见下).
    //   +3: comment.object_id (此前库里根本没有这个外键)、assets_tab.parent_uuid、
    //       work_xlsx_task_info_tab.project_id
    //   -3: 随废弃表一起消失的 ai_image_metadata.author 与 metadata_descriptor_department_link 的两个
    auto l_tables = scalar_int(
        l_session, "SELECT count(*) FROM sqlite_master WHERE type = 'table' AND name NOT LIKE 'sqlite_%';"
    );
    auto l_indexes = scalar_int(l_session, "SELECT count(*) FROM sqlite_master WHERE type = 'index';");
    auto l_fks     = scalar_int(
        l_session,
        R"(SELECT sum(c) FROM (SELECT (SELECT count(*) FROM pragma_foreign_key_list(m.name)) AS c
             FROM sqlite_master m WHERE m.type = 'table' AND m.name NOT LIKE 'sqlite_%');)"
    );
    BOOST_TEST_MESSAGE(fmt::format("升级后结构: 业务表={} 索引={} 外键={}", l_tables, l_indexes, l_fks));
    BOOST_TEST(l_tables == 73);
    BOOST_TEST(l_fks == 133);
    // 重建会删掉旧表上全部索引再按当前声明重建, 索引数只应减少
    BOOST_TEST(l_indexes < 262);

    // 两处可空可选归属的悬空引用必须被**置空**而不是删行 —— 升级前分别有 79 / 20 行
    for (const auto& [l_table, l_column, l_ref] :
         {std::tuple{"task", "last_preview_file_id", "preview_file"},
          std::tuple{"work_xlsx_task_info_tab", "kitsu_task_ref_id", "task"}}) {
      auto l_dangling = scalar_int(
          l_session,
          fmt::format(
              R"(SELECT count(*) FROM "{0}" WHERE "{1}" IS NOT NULL
                   AND NOT EXISTS (SELECT 1 FROM "{2}" WHERE "{2}"."uuid" = "{0}"."{1}");)",
              l_table, l_column, l_ref
          )
      );
      BOOST_TEST_MESSAGE(fmt::format("升级后 {}.{} 悬空引用 = {}", l_table, l_column, l_dangling));
      BOOST_TEST(l_dangling == 0);
    }
  }

  // 升级确实在动数据之前做了备份
  auto l_new_backups = [&]() {
    std::vector<std::string> l_result{};
    for (const auto& l_name : list_backup_files(l_backup_dir))
      if (!l_backup_before.contains(l_name)) l_result.push_back(l_name);
    return l_result;
  }();
  BOOST_TEST_MESSAGE(fmt::format("升级新建备份 {} 个", l_new_backups.size()));
  BOOST_TEST(l_new_backups.size() == 1);
  for (const auto& l_name : l_new_backups) {
    auto l_file = l_backup_dir / l_name;
    BOOST_TEST_MESSAGE(fmt::format("备份 {} = {:.1f} MB", l_name, FSys::file_size(l_file) / 1024.0 / 1024.0));
    std::error_code l_ec{};
    FSys::remove(l_file, l_ec);
  }

  remove_db(l_db);
}

BOOST_AUTO_TEST_SUITE_END()
