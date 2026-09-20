//
// 外键动作 (ON DELETE ...) 的行为断言.
//
// 这里验证的是"删除父行时到底会发生什么", 而不只是声明长什么样 —— 后者由 sqlite_conn_pragma
// 的 fk_declaration_invariants 覆盖. 两者都需要, 因为外键动作写错时 schema 完全合法,
// 只有在真正执行删除时才暴露 (例如把"必须无引用才可删"写成 CASCADE 会静默删掉业务数据).
//

#include <doodle_core/metadata/project.h>
#include <doodle_core/metadata/project_status.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <system_error>
#include <vector>

BOOST_AUTO_TEST_SUITE(sqlite_fk_semantics)

namespace {

using namespace doodle;
using namespace doodle::orm;

FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_fk_sem_{}.db", in_tag);
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

// 查询某表某列的外键删除动作 (pragma_foreign_key_list 的 on_delete)
std::string fk_on_delete(orm::session& in_session, const std::string& in_table, const std::string& in_column) {
  sqlite_stmt l_stmt{
      in_session,
      fmt::format(R"(SELECT on_delete FROM pragma_foreign_key_list('{}') WHERE "from" = '{}';)", in_table, in_column)
  };
  if (l_stmt.step_not_throw() != SQLITE_ROW) return "<不存在该外键>";
  return l_stmt.get_column_value<std::string>(0);
}

// 返回 true 表示删除被外键约束拒绝
bool try_delete_project_status(orm::session& in_session, const uuid& in_uuid) {
  try {
    delete_from(in_session).from<project_status>().where(c(&project_status::uuid_id_) == in_uuid)();
    return false;
  } catch (const std::exception&) {
    return true;
  }
}

}  // namespace

// 删除 project_status 只有在没有任何项目处于该状态时才允许.
// 原先声明为 ON DELETE CASCADE, 删一个项目状态会连带删掉该状态下的全部项目 ——
// 一次误删字典项就静默清空大批业务数据. 现在应为 no_action: 删除被拒绝, 由调用方先改项目状态.
BOOST_AUTO_TEST_CASE(deleting_project_status_in_use_is_rejected) {
  app_base l_app{};
  auto l_db = temp_db("project_status");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.sync_schema();

    const auto l_in_use = core_set::get_set().get_uuid();
    const auto l_free   = core_set::get_set().get_uuid();

    insert(l_session)
        .into<project_status>()
        .set(
            c(&project_status::uuid_id_) = l_in_use, c(&project_status::name_) = "in_use",
            c(&project_status::color_) = "#111111"
        )();
    insert(l_session)
        .into<project_status>()
        .set(
            c(&project_status::uuid_id_) = l_free, c(&project_status::name_) = "free",
            c(&project_status::color_) = "#222222"
        )();
    insert(l_session)
        .into<project>()
        .set(
            c(&project::uuid_id_) = core_set::get_set().get_uuid(), c(&project::name_) = "p1",
            c(&project::project_status_id_) = l_in_use
        )();

    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM project_status;") == 2);
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM project;") == 1);

    // 有项目正在使用 → 必须拒绝, 且项目与状态都不能少
    const bool l_rejected = try_delete_project_status(l_session, l_in_use);
    BOOST_TEST_MESSAGE("删除在用状态的 project_status 被拒绝: " + std::string{l_rejected ? "是" : "否"});
    BOOST_TEST(l_rejected);
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM project_status;") == 2);
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM project;") == 1);

    // 没有项目在使用 → 允许删除
    const bool l_free_rejected = try_delete_project_status(l_session, l_free);
    BOOST_TEST(l_free_rejected == false);
    BOOST_TEST(scalar_int(l_session, "SELECT count(*) FROM project_status;") == 1);
  }

  remove_db(l_db);
}

// 字典表的外键必须是 NO ACTION, 链接表与业务表的外键必须保持 CASCADE.
//
// 两类一起断言, 是为了防止"改字典表时顺手把链接表也改了": 链接表如果用 NO ACTION, 删字典项时
// 会被它自己的关联行挡住, 字典项反而永远删不掉 —— 这是把规则用错方向的典型后果.
BOOST_AUTO_TEST_CASE(dictionary_foreign_keys_are_no_action) {
  app_base l_app{};
  auto l_db = temp_db("dict_fk");
  remove_db(l_db);

  {
    sqlite_storage l_storage{};
    l_storage.open(l_db);
    auto l_session = l_storage.create_session();
    l_session.sync_schema();

    struct expect_t {
      const char* table_;
      const char* column_;
      const char* on_delete_;
    };
    const std::vector<expect_t> l_expect{
        // 字典数据 -> 业务数据: 有业务数据引用就不允许删除
        {       "task",           "task_type_id", "NO ACTION"},
        {       "task",         "task_status_id", "NO ACTION"},
        {     "entity",        "entity_type_id", "NO ACTION"},
        {   "playlist",         "task_type_id", "NO ACTION"},
        {    "comment",       "task_status_id", "NO ACTION"},
        {     "project",     "project_status_id", "NO ACTION"},
        {  "status_automation",    "in_task_type_id", "NO ACTION"},
        {  "status_automation",  "in_task_status_id", "NO ACTION"},
        {  "status_automation",   "out_task_type_id", "NO ACTION"},
        {  "status_automation", "out_task_status_id", "NO ACTION"},
        // 链接表: 链接行就是字典项与业务对象的关联, 删字典项时一并清理, 不该反过来阻止删除
        {"project_task_type_link",       "task_type_id",   "CASCADE"},
        {"project_task_status_link",   "task_status_id",   "CASCADE"},
        {"task_type_asset_type_link",    "asset_type_id",   "CASCADE"},
        // 业务数据 -> 业务数据: 子行离开父行没有意义, 保持级联删除
        {       "task",          "entity_id",   "CASCADE"},
        {     "entity",         "project_id",   "CASCADE"},
        {    "comment",          "object_id",   "CASCADE"},
        // 本次补齐的两个业务引用外键
        {  "assets_tab",        "parent_uuid",   "CASCADE"},  // 自引用树, 与 entity.parent_id 一致
        {"work_xlsx_task_info_tab",       "project_id",   "CASCADE"},
        // 可空的可选归属: 目标没了就把引用置空, 行本身保留 (绝不能是 CASCADE, 那会删掉有效业务行)
        {       "task", "last_preview_file_id", "SET NULL"},
        {"work_xlsx_task_info_tab", "kitsu_task_ref_id", "SET NULL"},
    };

    for (const auto& l_e : l_expect) {
      auto l_actual = fk_on_delete(l_session, l_e.table_, l_e.column_);
      BOOST_TEST_MESSAGE(
          fmt::format("{}.{}: on_delete = {} (期望 {})", l_e.table_, l_e.column_, l_actual, l_e.on_delete_)
      );
      BOOST_TEST(l_actual == l_e.on_delete_);
    }
  }

  remove_db(l_db);
}

BOOST_AUTO_TEST_SUITE_END()
