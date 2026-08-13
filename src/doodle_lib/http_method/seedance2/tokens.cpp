#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "reg.h"

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;
namespace {  // 设置所有人员当周剩余可使用的 token 数量,
boost::asio::awaitable<void> set_remaining_tokens_all_persons(std::int64_t in_tokens) {
  auto l_sql = get_sqlite_database();
  using namespace orm;

  co_await l_sql.update(
      orm::update(l_sql)
          .from<person>()
          .set(c(&person::remaining_completion_tokens_) = in_tokens)
          .where(c(&person::archived_) == false)
  );
  co_return;
}
// 设置当周人员剩余可使用的 token 数量
boost::asio::awaitable<void> set_remaining_tokens_for_person(const uuid& in_person, std::int64_t in_tokens) {
  if (in_tokens == 0) co_return;
  auto l_sql = get_sqlite_database();
  using namespace orm;

  co_await l_sql.update(
      orm::update(l_sql)
          .from<person>()
          .set(c(&person::remaining_completion_tokens_) = in_tokens)
          .where(c(&person::uuid_id_) == in_person)
  );

  co_return;
}
}  // namespace

// 设置当周人员剩余可使用的 token 数量
boost::asio::awaitable<void> add_remaining_tokens_for_person(const uuid& in_person, std::int64_t in_tokens) {
  if (in_tokens == 0) co_return;
  auto l_sql = get_sqlite_database();
  using namespace orm;

  co_await l_sql.update(
      orm::update(l_sql)
          .from<person>()
          .set(c(&person::remaining_completion_tokens_) = c(&person::remaining_completion_tokens_) + in_tokens)
          .where(c(&person::uuid_id_) == in_person)
  );

  co_return;
}
std::int64_t get_remaining_tokens_for_person(const uuid& in_person) {
  auto l_sql = get_sqlite_database();
  using namespace orm;
  return select(l_sql)
      .columns(&person::remaining_completion_tokens_)
      .from<person>()
      .where(c(&person::uuid_id_) == in_person)()
      .to_optional()
      .value_or(0);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens, get) {
  co_return in_handle->make_msg(nlohmann::json{{"remaining_tokens", person_.person_.remaining_completion_tokens_}});
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens, post) {
  // 设置当周人员剩余可使用的 token 数量
  person_.check_admin();
  auto l_json = in_handle->get_json();
  std::int64_t l_remaining_tokens{doodle_config::g_max_completion_tokens};
  if (l_json.contains("remaining_tokens")) l_remaining_tokens = l_json.at("remaining_tokens").get<std::int64_t>();
  co_await set_remaining_tokens_all_persons(l_remaining_tokens);
  co_return in_handle->make_msg(nlohmann::json{{"remaining_tokens", l_remaining_tokens}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_instance, get) {
  person_.check_producer();
  auto l_others_person = get_sqlite_database().get_by_uuid<person>(person_id_);
  co_return in_handle->make_msg(nlohmann::json{{"remaining_tokens", l_others_person.remaining_completion_tokens_}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_instance, put) {
  person_.check_admin();
  auto l_json = in_handle->get_json();
  if (!l_json.contains("remaining_tokens")) throw_exception(doodle_error{"缺少remaining_tokens字段"});
  std::int64_t l_remaining_tokens = l_json.at("remaining_tokens").get<std::int64_t>();
  auto l_others_person            = get_sqlite_database().get_by_uuid<person>(person_id_);
  co_await set_remaining_tokens_for_person(l_others_person.uuid_id_, l_remaining_tokens);  // 计算差值进行更新
  co_return in_handle->make_msg(nlohmann::json{{"remaining_tokens", l_remaining_tokens}});
}
namespace {

struct person_token_t {
  uuid person_id_;
  std::int64_t remaining_tokens_;  // 剩余可用token数量
                                   // to json
  friend void to_json(nlohmann::json& j, const person_token_t& p) {
    j["person_id"]        = p.person_id_;
    j["remaining_tokens"] = p.remaining_tokens_;
  }
};
}  // namespace
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_all, get) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  using namespace orm;

  std::vector<person_token_t> l_result_map;
  for (const auto& [person_id, max_tokens] : select(l_sql)
                                                 .columns(&person::uuid_id_, &person::remaining_completion_tokens_)
                                                 .from<person>()
                                                 .where(c(&person::archived_) == false)()) {
    l_result_map.push_back(person_token_t{person_id, max_tokens});
  }

  co_return in_handle->make_msg(nlohmann::json{} = l_result_map);
}
}  // namespace doodle::http::seedance2