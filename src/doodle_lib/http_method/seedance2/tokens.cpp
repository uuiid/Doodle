#include "doodle_core/doodle_core_fwd.h"
#include "doodle_core/exception/exception.h"
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/seedance2/task.h>

#include <doodle_lib/http_method/seedance2/reg.h>
#include <doodle_lib/sqlite_orm/orm/orm.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "core/global_function.h"
#include "reg.h"
#include <chrono>
#include <map>
#include <memory>
#include <set>
#include <utility>
#include <vector>

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
orm::update_t add_remaining_tokens_for_person(sqlite_database& in_sql, const uuid& in_person, std::int64_t in_tokens) {
  DOODLE_CHICK(in_tokens != 0, "in_tokens must not be 0");
  using namespace orm;

  return orm::update(in_sql)
      .from<person>()
      .set(c(&person::remaining_completion_tokens_) = c(&person::remaining_completion_tokens_) + in_tokens)
      .where(c(&person::uuid_id_) == in_person);
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

namespace {
auto get_date_person_tokens(const decltype(get_sqlite_database())& in_sql, const chrono::sys_days& in_date) {
  std::map<uuid, sd2::person_token> l_token_map;
  std::map<uuid, std::set<uuid>> l_project_map;
  using namespace orm;
  namespace sd2 = doodle::seedance2;

  chrono::system_zoned_time l_begin{chrono::current_zone(), chrono::sys_days{in_date}};
  chrono::system_zoned_time l_end{chrono::current_zone(), chrono::sys_days{in_date} + chrono::days{1}};
  for (auto&& [l_user_id, l_tokens, l_type, l_project_id] :
       select(in_sql)
           .columns(
               &sd2::task::user_id_, &sd2::task::completion_tokens_, &sd2::task::type_, &sd2::task::project_uuid_id_
           )
           .from<sd2::task>()
           .where(c(&sd2::task::created_at_) >= l_begin && c(&sd2::task::created_at_) < l_end)()) {
    if (l_token_map.contains(l_user_id)) {
      l_token_map.at(l_user_id).token_consumed_ += l_tokens;
      ++l_token_map.at(l_user_id).task_count_;
      if (!l_project_map[l_user_id].contains(l_project_id)) {
        ++l_token_map.at(l_user_id).project_count_;
        l_project_map.at(l_user_id).insert(l_project_id);
      }
      if (l_type == sd2::task_type::video) ++l_token_map.at(l_user_id).video_count_;
      if (l_type == sd2::task_type::picture) ++l_token_map.at(l_user_id).picture_count_;
    } else {
      l_token_map.emplace(
          l_user_id, sd2::person_token{
                         .date_           = chrono::year_month_day{in_date},
                         .person_id_      = l_user_id,
                         .token_consumed_ = l_tokens,
                         .task_count_     = 1,
                         .project_count_  = 1,
                         .video_count_    = (l_type == sd2::task_type::video ? 1 : 0),
                         .picture_count_  = (l_type == sd2::task_type::picture ? 1 : 0),
                     }
      );
      l_project_map[l_user_id].insert(l_project_id);
    }
  }
  return l_token_map;
}
}  // namespace

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_date_instance, get) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  using namespace orm;
  namespace sd2 = doodle::seedance2;
  std::vector<sd2::person_token> l_result_map =
      select(l_sql)
          .columns(object<sd2::person_token>())
          .from<sd2::person_token>()
          .where(c(&sd2::person_token::date_) >= date_start_ && c(&sd2::person_token::date_) <= date_end_ && c(&sd2::person_token::person_id_) == person_id_)()
          .to_vector();

  // 检查是否包含当天
  if (chrono::sys_days l_now{chrono::floor<chrono::days>(chrono::system_clock::now())};
      l_now >= date_start_ && l_now <= date_end_) {
    auto l_today_tokens = get_date_person_tokens(l_sql, l_now);
    if (l_today_tokens.contains(person_id_))
      l_result_map.push_back(l_today_tokens.at(person_id_));  // 添加当天的token消耗量
  }

  co_return in_handle->make_msg(nlohmann::json{} = l_result_map);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_date_instance_day, get) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  using namespace orm;
  namespace sd2 = doodle::seedance2;
  std::vector<sd2::person_token> l_result_map;

  // 检查是否包含当天
  if (chrono::sys_days l_now{chrono::floor<chrono::days>(chrono::system_clock::now())}; l_now == date_) {
    auto l_today_tokens = get_date_person_tokens(l_sql, l_now);
    if (l_today_tokens.contains(person_id_))
      l_result_map.push_back(l_today_tokens.at(person_id_));  // 添加当天的token消耗量
  } else {
    l_result_map =
        select(l_sql)
            .columns(object<sd2::person_token>())
            .from<sd2::person_token>()
            .where(c(&sd2::person_token::date_) == date_ && c(&sd2::person_token::person_id_) == person_id_)()
            .to_vector();
  }

  co_return in_handle->make_msg(nlohmann::json{} = l_result_map);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_date_all, get) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  using namespace orm;
  namespace sd2 = doodle::seedance2;
  std::vector<sd2::person_token> l_result_map =
      select(l_sql)
          .columns(object<sd2::person_token>())
          .from<sd2::person_token>()
          .where(c(&sd2::person_token::date_) >= date_start_ && c(&sd2::person_token::date_) <= date_end_)()
          .to_vector();
  // 检查是否包含当天
  if (chrono::sys_days l_now{chrono::floor<chrono::days>(chrono::system_clock::now())};
      l_now >= date_start_ && l_now <= date_end_) {
    auto l_today_tokens = get_date_person_tokens(l_sql, l_now);
    l_result_map |= ranges::actions::push_back(l_today_tokens | std::views::values);
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_result_map);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_date, get) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  using namespace orm;
  namespace sd2 = doodle::seedance2;
  std::vector<sd2::person_token> l_result_map;

  // 检查是否包含当天
  if (chrono::sys_days l_now{chrono::floor<chrono::days>(chrono::system_clock::now())}; l_now == date_) {
    auto l_today_tokens = get_date_person_tokens(l_sql, l_now);
    l_result_map        = l_today_tokens | std::views::values | ranges::to<std::vector<sd2::person_token>>();
  } else {
    l_result_map = select(l_sql)
                       .columns(object<sd2::person_token>())
                       .from<sd2::person_token>()
                       .where(c(&sd2::person_token::date_) == date_)()
                       .to_vector();
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_result_map);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(seedance2_tokens_person_date, post) {
  person_.check_admin();
  auto l_sql = get_sqlite_database();
  using namespace orm;
  namespace sd2 = doodle::seedance2;

  if (std::vector<sd2::person_token> l_result_map = select(l_sql)
                                                        .columns(object<sd2::person_token>())
                                                        .from<sd2::person_token>()
                                                        .where(c(&sd2::person_token::date_) == date_)()
                                                        .to_vector();
      !l_result_map.empty())
    co_return in_handle->make_msg(nlohmann::json{} = l_result_map);

  std::map<uuid, sd2::person_token> l_token_map = get_date_person_tokens(l_sql, date_);
  // 是当天的, 不进行插入, 直接返计算结果
  if (chrono::sys_days l_now{chrono::floor<chrono::days>(chrono::system_clock::now())}; l_now == date_)
    co_return in_handle->make_msg(
        nlohmann::json{} = l_token_map | std::views::values | ranges::to<std::vector<sd2::person_token>>()
    );

  auto l_list_vector = std::make_shared<std::vector<sd2::person_token>>();
  for (auto&& [l_person_id, l_tokens] : l_token_map) l_list_vector->emplace_back(l_tokens);
  co_await l_sql.install_range(l_list_vector);

  co_return in_handle->make_msg(nlohmann::json{} = *l_list_vector);
}
}  // namespace doodle::http::seedance2