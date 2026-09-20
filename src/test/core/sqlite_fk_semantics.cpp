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

BOOST_AUTO_TEST_SUITE_END()
