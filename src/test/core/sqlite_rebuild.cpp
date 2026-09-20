//
// sqlite_storage::rebuild_all_tables 测试
//

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/test/unit_test.hpp>

#include <cstdint>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(sqlite_rebuild)

namespace {

using namespace doodle;
using namespace doodle::orm;

struct rb_parent {
  std::int64_t id_{};
  std::string name_;
};
struct rb_child {
  std::int64_t id_{};
  std::int64_t parent_id_{};
  std::string note_;
};
// 用于验证触发器行为的审计表
struct rb_audit {
  std::int64_t parent_id_{};
};

// 注册两张测试表, 必须在 create_table() 之前调用
void reg_test_tables(sqlite_storage& in_storage) {
  in_storage.reg_table<rb_parent>("rb_parent")
      .add_column("id", &rb_parent::id_, primary_key())
      .add_column("name", &rb_parent::name_);
  in_storage.reg_table<rb_child>("rb_child")
      .add_column("id", &rb_child::id_, primary_key())
      .add_column("parent_id", &rb_child::parent_id_)
      .add_column("note", &rb_child::note_)
      .add_foreign_key(&rb_child::parent_id_, &rb_parent::id_);
}

void seed(orm::session& in_session) {
  in_session.create_table<rb_parent>();
  in_session.create_table<rb_child>();
  insert(in_session).into<rb_parent>().set(c(&rb_parent::id_) = 1, c(&rb_parent::name_) = "p1")();
  insert(in_session).into<rb_parent>().set(c(&rb_parent::id_) = 2, c(&rb_parent::name_) = "p2")();
  insert(in_session)
      .into<rb_child>()
      .set(c(&rb_child::id_) = 10, c(&rb_child::parent_id_) = 1, c(&rb_child::note_) = "n10")();
  insert(in_session)
      .into<rb_child>()
      .set(c(&rb_child::id_) = 11, c(&rb_child::parent_id_) = 2, c(&rb_child::note_) = "n11")();
}

// 是否存在残留的 <name>_backup 中间表
bool has_backup_table(orm::session& in_session, const std::string& in_name) {
  return in_session.get_all_table_names().contains(in_name + "_backup");
}

}  // namespace

// 核心: 重建后数据与表结构都保持完整, 中间表被清理, 外键约束恢复
BOOST_AUTO_TEST_CASE(rebuild_all_tables_preserves_data) {
  app_base l_app{};
  sqlite_storage l_storage{};
  l_storage.open();  // :memory:
  reg_test_tables(l_storage);
  auto l_session = l_storage.create_session();
  seed(l_session);

  // :memory: 库里只建了这两张表, regs_all() 注册的其余表都应被跳过
  auto l_count = l_storage.rebuild_all_tables(l_session);
  BOOST_TEST(l_count == 2);

  // 数据完整保留
  auto l_parents = select(l_session).columns(&rb_parent::id_, &rb_parent::name_).from<rb_parent>()().to_vector();
  BOOST_TEST(l_parents.size() == 2);
  auto l_child_ids = select(l_session).columns(&rb_child::id_).from<rb_child>()().to_vector();
  BOOST_TEST(l_child_ids.size() == 2);

  // 表仍可正常写入 (schema 未被破坏)
  insert(l_session).into<rb_parent>().set(c(&rb_parent::id_) = 3, c(&rb_parent::name_) = "p3")();
  BOOST_TEST(select(l_session).columns(&rb_parent::id_).from<rb_parent>()().to_vector().size() == 3);

  // 没有残留的 _backup 中间表
  BOOST_TEST(!has_backup_table(l_session, "rb_parent"));
  BOOST_TEST(!has_backup_table(l_session, "rb_child"));

  // 外键约束已恢复: 插入孤儿行应当失败
  BOOST_CHECK_THROW(
      insert(l_session)
          .into<rb_child>()
          .set(c(&rb_child::id_) = 99, c(&rb_child::parent_id_) = 9999, c(&rb_child::note_) = "orphan")(),
      std::exception
  );
}

// 重复执行应当幂等 (第二次同样成功且不留下中间表)
BOOST_AUTO_TEST_CASE(rebuild_all_tables_is_idempotent) {
  app_base l_app{};
  sqlite_storage l_storage{};
  l_storage.open();
  reg_test_tables(l_storage);
  auto l_session = l_storage.create_session();
  seed(l_session);

  BOOST_TEST(l_storage.rebuild_all_tables(l_session) == 2);
  BOOST_TEST(l_storage.rebuild_all_tables(l_session) == 2);

  BOOST_TEST(select(l_session).columns(&rb_parent::id_).from<rb_parent>()().to_vector().size() == 2);
  BOOST_TEST(select(l_session).columns(&rb_child::id_).from<rb_child>()().to_vector().size() == 2);
  BOOST_TEST(!has_backup_table(l_session, "rb_parent"));
  BOOST_TEST(!has_backup_table(l_session, "rb_child"));
}

// 空库 (只注册未建表) 应当安全跳过, 返回 0
BOOST_AUTO_TEST_CASE(rebuild_all_tables_on_empty_db) {
  app_base l_app{};
  sqlite_storage l_storage{};
  l_storage.open();
  reg_test_tables(l_storage);
  auto l_session = l_storage.create_session();

  BOOST_TEST(l_storage.rebuild_all_tables(l_session) == 0);
}

// 重建过程中复制数据的 INSERT ... SELECT 不得触发本表的 AFTER INSERT 触发器.
// 这是 entity / entity_fts 的场景: 若触发器在复制时触发, 会把每一行重复写进 FTS 索引.
BOOST_AUTO_TEST_CASE(rebuild_all_tables_does_not_fire_table_triggers) {
  app_base l_app{};
  sqlite_storage l_storage{};
  l_storage.open();
  reg_test_tables(l_storage);
  l_storage.reg_table<rb_audit>("rb_audit").add_column("parent_id", &rb_audit::parent_id_);
  auto l_session = l_storage.create_session();
  l_session.create_table<rb_parent>();
  l_session.create_table<rb_child>();
  l_session.create_table<rb_audit>();

  // 在 rb_parent 上挂一个 AFTER INSERT 触发器, 往审计表写一行
  auto l_trigger = l_storage.create_trigger("rb_parent_audit_trigger");
  l_trigger.after().insert().on<rb_parent>().begin().statement(
      insert(l_session).into<rb_audit>().set(c(&rb_audit::parent_id_) = new_(&rb_parent::id_))
  ).end();
  l_session.exec(l_trigger.to_sql(l_session, to_sql_ctx{.ctx_ = to_sql_ctx::create_trigger_sql}));

  insert(l_session).into<rb_parent>().set(c(&rb_parent::id_) = 1, c(&rb_parent::name_) = "p1")();
  insert(l_session).into<rb_parent>().set(c(&rb_parent::id_) = 2, c(&rb_parent::name_) = "p2")();
  BOOST_TEST(select(l_session).columns(&rb_audit::parent_id_).from<rb_audit>()().to_vector().size() == 2);

  l_storage.rebuild_all_tables(l_session);

  // 复制数据没有再次触发触发器
  BOOST_TEST(select(l_session).columns(&rb_audit::parent_id_).from<rb_audit>()().to_vector().size() == 2);
  // 数据仍在
  BOOST_TEST(select(l_session).columns(&rb_parent::id_).from<rb_parent>()().to_vector().size() == 2);
  // 触发器已恢复: 再插一行应当新增一条审计
  insert(l_session).into<rb_parent>().set(c(&rb_parent::id_) = 3, c(&rb_parent::name_) = "p3")();
  BOOST_TEST(select(l_session).columns(&rb_audit::parent_id_).from<rb_audit>()().to_vector().size() == 3);
}

BOOST_AUTO_TEST_SUITE_END()
