//
// pragma_foreign_key_check 违规行清理 与 同名表遮蔽 回归测试
//

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/scope/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(sqlite_fk_fix)

namespace {

using namespace doodle;
using namespace doodle::orm;

struct fk_parent {
  std::int64_t id_{};
  std::string name_;
};
struct fk_child {
  std::int64_t id_{};
  std::int64_t parent_id_{};
};

// 注册两张测试表, 必须在 create_table() 之前调用
void reg_test_tables(sqlite_storage& in_storage) {
  in_storage.reg_table<fk_parent>("fk_parent")
      .add_column("id", &fk_parent::id_, primary_key())
      .add_column("name", &fk_parent::name_);
  in_storage.reg_table<fk_child>("fk_child")
      .add_column("id", &fk_child::id_, primary_key())
      .add_column("parent_id", &fk_child::parent_id_)
      .add_foreign_key(&fk_child::parent_id_, &fk_parent::id_);
}

// 建表并插入 1 行合法子行 + 1 行违规子行 (parent_id = 999 不存在).
// 违规行是在模拟"外键约束尚未生效时期"遗留的历史脏数据, 所以必须在**关闭外键**的情况下写入:
// 开着外键时 SQLite 会直接拒绝这种写入 (该行为由 sqlite_conn_pragma 用例单独覆盖).
void seed(orm::session& in_session) {
  const auto l_fk_was_on = in_session.pragma().foreign_keys();
  in_session.pragma().foreign_keys(false);
  boost::scope::scope_exit l_fk_guard([&in_session, l_fk_was_on]() {
    in_session.pragma().foreign_keys(l_fk_was_on);
  });

  in_session.create_table<fk_parent>();
  in_session.create_table<fk_child>();
  insert(in_session).into<fk_parent>().set(c(&fk_parent::id_) = 1, c(&fk_parent::name_) = "p")();
  insert(in_session).into<fk_child>().set(c(&fk_child::id_) = 1, c(&fk_child::parent_id_) = 999)();
  insert(in_session).into<fk_child>().set(c(&fk_child::id_) = 2, c(&fk_child::parent_id_) = 1)();
}

}  // namespace

// 核心: fix_foreign_key_violations 删除违规行, 保留合法行
BOOST_AUTO_TEST_CASE(fix_foreign_key_violations_removes_orphans) {
  app_base l_app{};
  sqlite_storage l_storage{};
  l_storage.open();  // :memory:
  reg_test_tables(l_storage);
  auto l_session = l_storage.create_session();
  seed(l_session);

  BOOST_TEST(l_session.pragma().foreign_key_check().size() == 1);

  l_session.pragma().foreign_keys(false);
  {
    auto l_guard = l_session.transaction();
    l_storage.fix_foreign_key_violations(l_session);
    l_guard.commit();
  }
  l_session.pragma().foreign_keys(true);

  BOOST_TEST(l_session.pragma().foreign_key_check().empty());

  // 违规行 (id=1) 已删, 合法行 (id=2) 保留
  auto l_left = select(l_session).columns(&fk_child::id_).from<fk_child>()().to_vector();
  BOOST_TEST(l_left.size() == 1);
  if (!l_left.empty()) BOOST_TEST(l_left.front() == 2);
}

// 回归: 库中存在同名真实表时, foreign_key_check() 仍必须返回违规行.
// 证明走的是 PRAGMA 语句形式, 而不是会被遮蔽的 `SELECT ... FROM pragma_foreign_key_check`.
BOOST_AUTO_TEST_CASE(foreign_key_check_not_shadowed_by_real_table) {
  app_base l_app{};
  sqlite_storage l_storage{};
  l_storage.open();
  reg_test_tables(l_storage);
  auto l_session = l_storage.create_session();
  seed(l_session);

  // 故意造一张同名真实表来遮蔽 eponymous 虚拟表
  l_session.exec(
      R"(CREATE TABLE "pragma_foreign_key_check" ("table" TEXT, "rowid" INTEGER, "parent" TEXT, "fkid" INTEGER))"
  );

  // 此时 `SELECT ... FROM pragma_foreign_key_check` 会读到这张空表, 但 PRAGMA 语句形式不受影响
  BOOST_TEST(l_session.pragma().foreign_key_check().size() == 1);
}

BOOST_AUTO_TEST_SUITE_END()
