
#include "doodle_core/metadata/person.h"
#include <doodle_core/metadata/ai_studio.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>
#include <doodle_lib/sqlite_orm/sqlite_select_data.h>

#include "kitsu_reg_url.h"
#include <vector>

namespace doodle::http {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio, post) {
  auto l_sql       = get_sqlite_database();
  auto l_json      = in_handle->get_json();
  auto l_ai_studio = std::make_shared<ai_studio>();
  l_json.get_to(*l_ai_studio);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始创建 AI 工作室 name {}", person_.person_.email_,
      person_.person_.get_full_name(), l_ai_studio->name_
  );

  co_await l_sql.install(l_ai_studio);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成创建 AI 工作室 id {} name {}", person_.person_.email_,
      person_.person_.get_full_name(), l_ai_studio->uuid_id_, l_ai_studio->name_
  );

  co_return in_handle->make_msg(nlohmann::json{} = *l_ai_studio);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio, get) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  auto l_vec = l_sql.get_all<ai_studio>();
  co_return in_handle->make_msg(nlohmann::json{} = l_vec);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance, get) {
  person_.check_producer();
  auto l_sql       = get_sqlite_database();
  auto l_ai_studio = l_sql.get_by_uuid<ai_studio>(id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_ai_studio);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance, put) {
  person_.check_producer();
  auto l_sql  = get_sqlite_database();
  auto l_json = in_handle->get_json();

  using namespace orm;
  auto l_update = update(l_sql).from<ai_studio>().set_from_ref<ai_studio>(l_json).where(c(&ai_studio::uuid_id_) == id_);
  co_await l_sql.run_sql(l_update);

  auto l_ai_studio = l_sql.get_by_uuid<ai_studio>(id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成更新 AI 工作室 id {} name {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_ai_studio.name_
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_ai_studio);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance, delete_) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 AI 工作室 id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_
  );

  l_sql.uuid_to_id<ai_studio>(id_);
  co_await l_sql.remove<ai_studio>(id_);
  co_return in_handle->make_msg_204();
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance_person_instance, post) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();
  {
    using namespace orm;
    auto l_update = update(l_sql)
                        .from<person>()
                        .set(c(&person::ai_studio_id_) = ai_studio_id_)
                        .where(c(&person::uuid_id_) == person_id_);
    co_await l_sql.run_sql(l_update);
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成将人员 {} 加入 AI 工作室 {}", person_.person_.email_,
      person_.person_.get_full_name(), person_id_, ai_studio_id_
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_by_uuid<person>(person_id_));
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_ai_studio_instance_person_instance, delete_) {
  person_.check_producer();
  auto l_sql = get_sqlite_database();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 将人员 {} 从 AI 工作室 {} 移除", person_.person_.email_,
      person_.person_.get_full_name(), person_id_, ai_studio_id_
  );
  {
    using namespace orm;
    auto l_update =
        update(l_sql).from<person>().set(c(&person::ai_studio_id_) = uuid{}).where(c(&person::uuid_id_) == person_id_);
    co_await l_sql.run_sql(l_update);
  }
  co_return in_handle->make_msg(nlohmann::json{} = l_sql.get_by_uuid<person>(person_id_));
}
}  // namespace doodle::http