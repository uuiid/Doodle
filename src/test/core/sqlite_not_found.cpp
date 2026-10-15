//
// ORM「查不到行」的异常语义测试.
//
// 盯住一件事: to_single() 在结果集为空时抛的是 sqlite_orm_exception,
// 并且**自带 404 状态码**。HTTP 层 (http_session_data.cpp) 直接取这个状态码当响应码,
// 所以这条契约一旦破掉, 「取一个不存在的 id」就会退化成 500 或者 400 —— 两种都是错的。
//
// 为什么值得单独一份用例:
//   * 以前抛的是 std::runtime_error("No rows returned")，与 sqlite_orm_exception 是
//     **兄弟**关系（都直接继承 std::runtime_error），所以 catch 其中一个接不住另一个。
//     当时唯一的补救办法是在每个调用点包一层 try/catch 把「所有异常」翻成 404 ——
//     那顺手把「库真的坏了」也伪装成了「资源不存在」。
//   * 状态码是**行为**，不是注释。这里直接断言数值，而不是断言「抛了异常」。
//

#include <doodle_core/metadata/ai_light/ai_light.h>

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/core/core_set.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include <boost/scope/scope_exit.hpp>
#include <boost/test/unit_test.hpp>

#include <string>
#include <system_error>

BOOST_AUTO_TEST_SUITE(sqlite_not_found)

namespace {

using namespace doodle;
using namespace doodle::orm;
using namespace doodle::ai_light;

FSys::path temp_db(const std::string& in_tag) {
  return FSys::temp_directory_path() / fmt::format("doodle_test_sqlite_not_found_{}.db", in_tag);
}

void remove_db(const FSys::path& in_path) {
  std::error_code l_ec{};
  for (const auto* l_suffix : {"", "-wal", "-shm"}) FSys::remove(FSys::path{in_path.generic_string() + l_suffix}, l_ec);
}

/// 一个只有一行 ai_light_preset 的真库。
///
/// 用 ai_light_preset 而不是现造的测试表：它已经有 reg_table 注册与真实迁移路径，
/// 而这里的重点是「查不到」这件事本身，不该再引入一套只为测试存在的表定义。
struct fixture_t {
  sqlite_storage storage_{};
  orm::session orm_{};
  std::unique_ptr<sqlite_database> sql_{};
  FSys::path path_{};
  bool fk_was_on_{true};
  ai_light_preset preset_{};

  explicit fixture_t(const std::string& in_tag) : path_{temp_db(in_tag)} {
    remove_db(path_);
    storage_.open(path_);
    storage_.upgrade();
    orm_ = storage_.create_session();
    sql_ = std::make_unique<sqlite_database>(storage_.get_strand(), orm_);

    // 归属列是 NOT NULL 外键，指向并不存在的 person，所以构造期间关掉外键
    // （与 ai_light_schema.cpp / ai_light_state.cpp 的做法一致），析构时恢复。
    fk_was_on_ = orm_.pragma().foreign_keys();
    orm_.pragma().foreign_keys(false);

    preset_.name_    = "三点布光";
    preset_.enabled_ = true;
    insert(orm_).into<ai_light_preset>().values(preset_)();
  }

  ~fixture_t() {
    orm_.pragma().foreign_keys(fk_was_on_);
    sql_.reset();
    remove_db(path_);
  }

  fixture_t(const fixture_t&)            = delete;
  fixture_t& operator=(const fixture_t&) = delete;
};

}  // namespace

/// 查不到行 → sqlite_orm_exception，且状态码是 404（不是 500、也不是 400）
BOOST_AUTO_TEST_CASE(to_single_throws_not_found_with_404_status) {
  app_base l_app{};
  fixture_t l_fx{"not_found"};

  bool l_caught = false;
  try {
    auto l_row = l_fx.sql_->get_by_uuid<ai_light_preset>(core_set::get_set().get_uuid());
    (void)l_row;
  } catch (const sqlite_orm_exception& in_err) {
    l_caught = true;
    // 状态码就是 HTTP 响应码本身，所以这里断言的是**行为**而不是「抛了异常」
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_found);
    // 消息里要有表名：HTTP 层把它直接当 404 的响应体，而 "No rows returned"
    // 既看不出是哪张表，也看不出是「不存在」还是「查询写错了」
    BOOST_TEST(in_err.what() == std::string{"ai_light_preset 中查不到该行"});
  }
  BOOST_TEST(l_caught);

  // 必须是 sqlite_orm_exception，而不是别的 std::runtime_error 兄弟类型：
  // 否则 http_session_data 里那个 catch 接不住，又会退化成 500
  bool l_is_orm_exception = false;
  try {
    auto l_row = l_fx.sql_->get_by_uuid<ai_light_preset>(core_set::get_set().get_uuid());
    (void)l_row;
  } catch (const std::runtime_error& in_err) {
    l_is_orm_exception = dynamic_cast<const sqlite_orm_exception*>(&in_err) != nullptr;
  }
  BOOST_TEST(l_is_orm_exception);
}

/// 查得到行时一切照旧（别为了错误路径把正常路径改坏）
BOOST_AUTO_TEST_CASE(to_single_returns_the_row_when_present) {
  app_base l_app{};
  fixture_t l_fx{"present"};

  auto l_row = l_fx.sql_->get_by_uuid<ai_light_preset>(l_fx.preset_.uuid_id_);
  BOOST_TEST(l_row.uuid_id_ == l_fx.preset_.uuid_id_);
  BOOST_TEST(l_row.name_ == "三点布光");
}

/// where 条件命中 0 行时也是 404 —— 不只有 get_by_uuid 这一条路
BOOST_AUTO_TEST_CASE(empty_select_result_is_also_404) {
  app_base l_app{};
  fixture_t l_fx{"empty_select"};

  bool l_caught = false;
  try {
    auto l_row = select(*l_fx.sql_)
                     .columns(object<ai_light_preset>())
                     .from<ai_light_preset>()
                     .where(c(&ai_light_preset::name_) == std::string{"不存在的预设名"})()
                     .to_single();
    (void)l_row;
  } catch (const sqlite_orm_exception& in_err) {
    l_caught = true;
    BOOST_TEST(in_err.code_status_() == boost::beast::http::status::not_found);
  }
  BOOST_TEST(l_caught);
}

/// to_optional() 对空结果集仍然返回 nullopt —— 它是「允许没有」的显式通道，
/// 不能因为 to_single() 改了行为就跟着抛异常
BOOST_AUTO_TEST_CASE(to_optional_still_returns_nullopt_on_empty) {
  app_base l_app{};
  fixture_t l_fx{"optional"};

  auto l_row = select(*l_fx.sql_)
                   .columns(object<ai_light_preset>())
                   .from<ai_light_preset>()
                   .where(c(&ai_light_preset::name_) == std::string{"不存在的预设名"})()
                   .to_optional();
  BOOST_TEST(!l_row.has_value());
}

/// 默认状态码保持 bad_request：既有抛出点（约束冲突等）都是调用方输入问题，
/// 不能因为新加了状态码就把它们悄悄变成 500
BOOST_AUTO_TEST_CASE(default_status_stays_bad_request) {
  sqlite_orm_exception l_default{"随便一条消息"};
  BOOST_TEST(l_default.code_status_() == boost::beast::http::status::bad_request);

  rebind_range_size_mismatch_exception l_rebind{};
  BOOST_TEST(l_rebind.code_status_() == boost::beast::http::status::bad_request);
}

BOOST_AUTO_TEST_SUITE_END()
