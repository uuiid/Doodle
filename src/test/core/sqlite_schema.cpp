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
#include <any>
#include <fmt/format.h>

BOOST_AUTO_TEST_SUITE(sqlite_schema)

namespace {

using namespace doodle;
using namespace doodle::orm;

struct unary_note {
  std::int64_t id_{};
  std::string backend_;
  std::string model_name_;
  bool archived_{};
};

void reg_unary_note_table(sqlite_storage& in_storage) {
  in_storage.reg_table<unary_note>("unary_note")
      .add_column("id", &unary_note::id_, primary_key())
      .add_column("backend", &unary_note::backend_)
      .add_column("model_name", &unary_note::model_name_)
      .add_column("archived", &unary_note::archived_);
}

std::string where_sql_of(orm::session& in_session, const column_operations_base_t& in_condition) {
  to_sql_ctx l_ctx{};
  l_ctx.ctx_ = to_sql_ctx::where_sql;
  return in_condition.to_sql(in_session, l_ctx);
}

std::vector<std::string> bind_values_of(orm::session& in_session, const column_operations_base_t& in_condition) {
  auto l_collector = bind_value_collector_t{};
  in_condition.collect_bind_variants(l_collector);
  std::vector<std::string> l_ret{};
  for (const auto& l_bind : l_collector.bind_values_) {
    if (const auto* l_str = std::any_cast<std::string>(&l_bind.value_)) l_ret.emplace_back(*l_str);
  }
  return l_ret;
}

}  // namespace

// !c(&T::bool_) 只取反一个裸列, 不产生绑定参数
BOOST_AUTO_TEST_CASE(not_on_bare_column) {
  app_base l_app{};
  sqlite_storage l_storage{};
  l_storage.open();
  reg_unary_note_table(l_storage);
  auto l_session = l_storage.create_session();
  l_session.create_table<unary_note>();

  to_sql_ctx l_ctx{};
  l_ctx.ctx_ = to_sql_ctx::where_sql;
  const auto l_column_name = l_session.get_column_name(&unary_note::archived_, l_ctx);

  auto l_condition = !c(&unary_note::archived_);

  BOOST_TEST(where_sql_of(l_session, l_condition) == fmt::format("NOT ({})", l_column_name));
  BOOST_TEST(bind_values_of(l_session, l_condition).empty());
}

// !(c(...).model_name_.like(...)) 取反整个 LIKE 表达式, 必须保留 LIKE 与绑定参数
BOOST_AUTO_TEST_CASE(not_on_like_expression) {
  app_base l_app{};
  sqlite_storage l_storage{};
  l_storage.open();
  reg_unary_note_table(l_storage);
  auto l_session = l_storage.create_session();
  l_session.create_table<unary_note>();

  to_sql_ctx l_ctx{};
  l_ctx.ctx_ = to_sql_ctx::where_sql;
  const auto l_column_name = l_session.get_column_name(&unary_note::model_name_, l_ctx);

  auto l_condition = !(c(&unary_note::model_name_).like("doubao-%"));

  BOOST_TEST(where_sql_of(l_session, l_condition) == fmt::format("NOT ({} LIKE ?)", l_column_name));
  const auto l_binds = bind_values_of(l_session, l_condition);
  BOOST_REQUIRE_EQUAL(l_binds.size(), 1);
  BOOST_TEST(l_binds.front() == "doubao-%");
}

BOOST_AUTO_TEST_SUITE_END()
