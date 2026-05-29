#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "reg.h"

namespace doodle::http::seedance2 {
namespace sd2 = doodle::seedance2;

// 设置当日人员剩余可使用的 token 数量
boost::asio::awaitable<void> set_remaining_tokens_for_person(const person& in_person, std::int64_t in_tokens) {
  if (in_tokens == 0) co_return;
  auto& l_sql = get_sqlite_database();
  using namespace orm;
  chrono::year_month_day l_today = chrono::floor<chrono::days>(chrono::system_clock::now());

  auto l_token_record =
      select(l_sql)
          .columns(object<sd2::task_person_token>())
          .from<sd2::task_person_token>()
          .where(c(&sd2::task_person_token::person_id_) == in_person.uuid_id_ && c(&sd2::task_person_token::token_usage_date_) == l_today)()
          .to_optional();
  if (!l_token_record) {
    auto l_new_record               = std::make_shared<sd2::task_person_token>();
    l_new_record->person_id_        = in_person.uuid_id_;
    l_new_record->remaining_tokens_ = in_person.max_completion_tokens_;
    l_new_record->token_usage_date_ = l_today;
    co_await l_sql.install(l_new_record);
  }

  co_await l_sql.update(
      orm::update(l_sql)
          .from<sd2::task_person_token>()
          .set(c(&sd2::task_person_token::remaining_tokens_) = in_tokens)
          .where(
              c(&sd2::task_person_token::person_id_) == in_person.uuid_id_ &&
              c(&sd2::task_person_token::token_usage_date_) == l_today
          )
  );

  co_return;
}

// 设置当日人员剩余可使用的 token 数量
boost::asio::awaitable<void> add_remaining_tokens_for_person(const person& in_person, std::int64_t in_tokens) {
  if (in_tokens == 0) co_return;
  auto& l_sql = get_sqlite_database();
  using namespace orm;
  chrono::year_month_day l_today = chrono::floor<chrono::days>(chrono::system_clock::now());

  auto l_token_record =
      select(l_sql)
          .columns(object<sd2::task_person_token>())
          .from<sd2::task_person_token>()
          .where(c(&sd2::task_person_token::person_id_) == in_person.uuid_id_ && c(&sd2::task_person_token::token_usage_date_) == l_today)()
          .to_optional();
  if (!l_token_record) {
    auto l_new_record               = std::make_shared<sd2::task_person_token>();
    l_new_record->person_id_        = in_person.uuid_id_;
    l_new_record->remaining_tokens_ = in_person.max_completion_tokens_;
    l_new_record->token_usage_date_ = l_today;
    co_await l_sql.install(l_new_record);
  }

  co_await l_sql.update(
      orm::update(l_sql)
          .from<sd2::task_person_token>()
          .set(
              c(&sd2::task_person_token::remaining_tokens_) =
                  in_tokens > 0 ? c(&sd2::task_person_token::remaining_tokens_) + in_tokens
                                : c(&sd2::task_person_token::remaining_tokens_) - in_tokens
          )
          .where(
              c(&sd2::task_person_token::person_id_) == in_person.uuid_id_ &&
              c(&sd2::task_person_token::token_usage_date_) == l_today
          )
  );

  co_return;
}
std::int64_t get_remaining_tokens_for_person(const person& in_person) {
  auto& l_sql = get_sqlite_database();
  using namespace orm;
  chrono::year_month_day l_today = chrono::floor<chrono::days>(chrono::system_clock::now());
  auto l_tokens =
      select(l_sql)
          .columns(&sd2::task_person_token::remaining_tokens_)
          .from<sd2::task_person_token>()
          .where(c(&sd2::task_person_token::person_id_) == in_person.uuid_id_ && c(&sd2::task_person_token::token_usage_date_) == l_today)()
          .to_optional();
  if (l_tokens) return l_tokens.value();
  return in_person.max_completion_tokens_;
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens, get) {
  co_return in_handle->make_msg(nlohmann::json{{"remaining_tokens", get_remaining_tokens_for_person(person_.person_)}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_instance, get) {
  person_.check_producer();
  auto l_others_person = get_sqlite_database().get_by_uuid<person>(person_id_);
  co_return in_handle->make_msg(nlohmann::json{{"remaining_tokens", get_remaining_tokens_for_person(l_others_person)}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_instance, put) {
  person_.check_admin();
  auto l_json = in_handle->get_json();
  if (!l_json.contains("remaining_tokens")) throw_exception(doodle_error{"缺少remaining_tokens字段"});
  std::int64_t l_remaining_tokens = l_json.at("remaining_tokens").get<std::int64_t>();
  auto l_others_person            = get_sqlite_database().get_by_uuid<person>(person_id_);
  co_await set_remaining_tokens_for_person(l_others_person, l_remaining_tokens);  // 计算差值进行更新
  co_return in_handle->make_msg(nlohmann::json{{"remaining_tokens", get_remaining_tokens_for_person(l_others_person)}});
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_all, get) {
  person_.check_producer();
  auto& l_sql = get_sqlite_database();
  using namespace orm;

  std::map<uuid, std::int64_t> l_result_map{};
  for (const auto& [person_id, max_tokens] : select(l_sql)
                                                 .columns(&person::uuid_id_, &person::max_completion_tokens_)
                                                 .from<person>()
                                                 .where(c(&person::archived_) == false)()) {
    l_result_map[person_id] = max_tokens;
  }

  chrono::year_month_day l_today = chrono::floor<chrono::days>(chrono::system_clock::now());
  auto l_result                  = select(l_sql)
                      .columns(object<sd2::task_person_token>())
                      .from<sd2::task_person_token>()
                      .where(c(&sd2::task_person_token::token_usage_date_) == l_today)()
                      .to_vector();
  for (auto& item : l_result)
    if (l_result_map.contains(item.person_id_)) l_result_map.erase(item.person_id_);
  for (const auto& [person_id, max_tokens] : l_result_map) {
    l_result.push_back(
        sd2::task_person_token{
            .person_id_        = person_id,
            .remaining_tokens_ = max_tokens,
            .token_usage_date_ = l_today,
        }
    );
  }

  co_return in_handle->make_msg(nlohmann::json{} = l_result);
}
}  // namespace doodle::http::seedance2