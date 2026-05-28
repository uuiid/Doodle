//
// Created by TD on 24-8-21.
//

#include "doodle_core/metadata/notification.h"
#include "doodle_core/metadata/project.h"
#include <doodle_core/exception/exception.h>
#include <doodle_core/metadata/department.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/person.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/status_automation.h>
#include <doodle_core/metadata/studio.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/metadata/user.h>

#include <doodle_lib/core/bcrypt/bcrypt.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>
#include <doodle_lib/sqlite_orm/sqlite_database.h>

#include "kitsu_reg_url.h"
#include "sqlite_orm/orm/select.h"
#include <core/http/http_function.h>
#include <jwt-cpp/traits/nlohmann-json/traits.h>
#include <string>

namespace doodle::http {
namespace {
std::vector<project_with_extra_data> get_project_for_user(const http_jwt_fun::http_jwt_t& in_user) {
  auto& l_sql = get_sqlite_database();
  using namespace orm;
  std::vector<project_with_extra_data> l_projects{};
  auto l_select = select(l_sql).columns(object<project>());
  l_select.from<project>();
  l_select.join<project_status>(&project::project_status_id_, &project_status::uuid_id_);
  if (in_user.is_admin()) {
    l_select.where(c(&project_status::name_) == "Open");
  } else {
    l_select.join<project_person_link>(&project::uuid_id_, &project_person_link::project_id_)
        .where(c(&project_person_link::person_id_) == in_user.person_.uuid_id_ && c(&project_status::name_) == "Open");
  }
  for (auto&& l_project : l_select()) l_projects.push_back(project_with_extra_data{l_project});
  for (auto& l_project : l_projects) {
    l_project.team_ = select(l_sql)
                          .columns(&project_person_link::person_id_)
                          .from<project_person_link>()
                          .where(c(&project_person_link::project_id_) == l_project.uuid_id_)()
                          .to_vector();
    l_project.asset_types_ = select(l_sql)
                                 .columns(&project_asset_type_link::asset_type_id_)
                                 .from<project_asset_type_link>()
                                 .where(c(&project_asset_type_link::project_id_) == l_project.uuid_id_)()
                                 .to_vector();
    l_project.task_statuses_ = select(l_sql)
                                   .columns(&project_task_status_link::task_status_id_)
                                   .from<project_task_status_link>()
                                   .where(c(&project_task_status_link::project_id_) == l_project.uuid_id_)()
                                   .to_vector();
    l_project.task_types_ = select(l_sql)
                                .columns(&project_task_type_link::task_type_id_)
                                .from<project_task_type_link>()
                                .where(c(&project_task_type_link::project_id_) == l_project.uuid_id_)()
                                .to_vector();
    l_project.status_automations_ = select(l_sql)
                                        .columns(&project_status_automation_link::status_automation_id_)
                                        .from<project_status_automation_link>()
                                        .where(c(&project_status_automation_link::project_id_) == l_project.uuid_id_)()
                                        .to_vector();
    l_project.preview_background_files_ =
        select(l_sql)
            .columns(&project_preview_background_file_link::preview_background_file_id_)
            .from<project_preview_background_file_link>()
            .where(c(&project_preview_background_file_link::project_id_) == l_project.uuid_id_)()
            .to_vector();
    l_project.task_types_priority_ = select(l_sql)
                                         .columns(object<project_task_type_link>())
                                         .from<project_task_type_link>()
                                         .where(c(&project_task_type_link::project_id_) == l_project.uuid_id_)()
                                         .to_vector();
    l_project.task_statuses_link_ = select(l_sql)
                                        .columns(object<project_task_status_link>())
                                        .from<project_task_status_link>()
                                        .where(c(&project_task_status_link::project_id_) == l_project.uuid_id_)()
                                        .to_vector();
    l_project.episodes_ = select(l_sql)
                              .columns(&entity_asset_extend::ji_shu_lie_, count())
                              .from<entity_asset_extend>()
                              .join<entity>(&entity_asset_extend::entity_id_, &entity::uuid_id_)
                              .where(c(&entity::project_id_) == l_project.uuid_id_)
                              .group_by(&entity_asset_extend::ji_shu_lie_)()
                              .to_vector<project_with_extra_data::project_int>();
    l_project.seasons_ = select(l_sql)
                             .columns(&entity_asset_extend::ji_du_, count())
                             .from<entity_asset_extend>()
                             .join<entity>(&entity_asset_extend::entity_id_, &entity::uuid_id_)
                             .where(c(&entity::project_id_) == l_project.uuid_id_)
                             .group_by(&entity_asset_extend::ji_du_)()
                             .to_vector<project_with_extra_data::project_int>();
    l_project.levels_ = select(l_sql)
                            .columns(&entity_asset_extend::deng_ji_, count())
                            .from<entity_asset_extend>()
                            .join<entity>(&entity_asset_extend::entity_id_, &entity::uuid_id_)
                            .where(c(&entity::project_id_) == l_project.uuid_id_)
                            .group_by(&entity_asset_extend::deng_ji_)()
                            .to_vector<project_with_extra_data::project_str>();
    l_project.scenes_ = select(l_sql)
                            .columns(&entity_asset_extend::gui_dang_, count())
                            .from<entity_asset_extend>()
                            .join<entity>(&entity_asset_extend::entity_id_, &entity::uuid_id_)
                            .where(c(&entity::project_id_) == l_project.uuid_id_)
                            .group_by(&entity_asset_extend::gui_dang_)()
                            .to_vector<project_with_extra_data::project_int>();
  }

  return l_projects;
}

}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> user_context::get(session_data_ptr in_handle) {
  nlohmann::json l_ret{};
  using namespace orm;
  auto& l_sql             = get_sqlite_database();
  l_ret["asset_types"]    = l_sql.get_asset_types_not_temporal_type();
  l_ret["custom_actions"] = nlohmann::json::value_t::array;
  l_ret["departments"]    = l_sql.get_all<department>();
  {
    auto l_notifications = select(l_sql).columns(count(&notification::uuid_id_));
    l_notifications.from<notification>().where(c(&notification::person_id_) == person_.person_.uuid_id_);
    l_ret["notification_count"] = l_notifications().to_single();
  }
  {
    auto l_persons = l_sql.get_all<person>();
    for (auto&& l_person : l_persons) {
      l_person.departments_ = select(l_sql)
                                  .columns(&person_department_link::department_id_)
                                  .from<person_department_link>()
                                  .where(c(&person_department_link::person_id_) == l_person.uuid_id_)()
                                  .to_vector();
    }
    l_ret["persons"] = l_persons;
  }
  l_ret["project_status"]           = l_sql.get_all<project_status>();
  l_ret["preview_background_files"] = nlohmann::json::value_t::array;
  l_ret["projects"]                 = get_project_for_user(person_);
  l_ret["status_automations"]       = l_sql.get_all<status_automation>();
  l_ret["studios"]                  = l_sql.get_all<studio>();
  l_ret["task_status"]              = l_sql.get_all<task_status>();
  l_ret["task_types"]               = l_sql.get_all<task_type>();
  l_ret["user_limit"]               = 9999;
  l_ret["search_filter_groups"]     = nlohmann::json::value_t::object;
  l_ret["search_filters"]           = nlohmann::json::value_t::object;

  co_return in_handle->make_msg(l_ret);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_person::get(session_data_ptr in_handle) {
  auto l_p = get_sqlite_database().get_all<person>();
  co_return in_handle->make_msg(nlohmann::json{} = l_p);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_person::post(session_data_ptr in_handle) {
  person_.check_admin();
  auto l_person       = std::make_shared<person>(in_handle->get_json().get<person>());
  l_person->timezone_ = chrono::current_zone()->name();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始创建用户 email {}", person_.person_.email_,
      person_.person_.get_full_name(), l_person->email_
  );

  co_await get_sqlite_database().install(l_person);
  auto l_person_deps = std::make_shared<std::vector<person_department_link>>();
  for (auto&& l_dep : l_person->departments_) {
    l_person_deps->emplace_back(person_department_link{.person_id_ = l_person->uuid_id_, .department_id_ = l_dep});
  }
  co_await get_sqlite_database().install_range(l_person_deps);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成创建用户 person_id {} email {} 部门数量 {}", person_.person_.email_,
      person_.person_.get_full_name(), l_person->uuid_id_, l_person->email_, l_person->departments_.size()
  );

  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_person_instance::put(session_data_ptr in_handle) {
  auto& l_sql       = get_sqlite_database();
  auto l_old_person = l_sql.get_by_uuid<person>(id_);
  auto l_person     = std::make_shared<person>(l_old_person);
  auto l_json       = in_handle->get_json();
  l_json.get_to(*l_person);
  if (l_old_person.studio_id_ != l_person->studio_id_ && !l_person->phone_.empty()) {
    auto l_studio          = l_sql.get_by_uuid<studio>(l_person->studio_id_);
    auto l_dingding_client = g_ctx().get<dingding::dingding_company>().make_client(l_studio);
    l_person->dingding_id_ = co_await l_dingding_client->get_user_by_mobile(l_person->phone_);
  }

  co_await l_sql.update(l_person);
  if (l_old_person.departments_ != l_person->departments_) {
    using namespace orm;
    co_await l_sql.remove(
        orm::delete_from(l_sql).from<person_department_link>().where(
            c(&person_department_link::person_id_) == l_person->uuid_id_
        )
    );
    std::shared_ptr<std::vector<person_department_link>> l_person_deps =
        std::make_shared<std::vector<person_department_link>>();
    for (auto&& l_dep : l_person->departments_) {
      l_person_deps->emplace_back(person_department_link{.person_id_ = l_person->uuid_id_, .department_id_ = l_dep});
    }
    co_await l_sql.install_range(l_person_deps);
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成更新用户 person_id {} email {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_person->email_
  );

  nlohmann::json l_ret{};
  l_ret = *l_person;

  if (l_json.contains("expiration_date")) {
    auto& l_ctx           = g_ctx().get<kitsu_ctx_t>();
    l_ret["access_token"] = jwt::create()
                                .set_payload_claim("identity_type", jwt::claim{"person"s})
                                .set_issued_at(chrono::system_clock::now())
                                .set_id(fmt::to_string(person_.person_.uuid_id_))
                                .set_subject(fmt::to_string(person_.person_.uuid_id_))
                                .set_not_before(chrono::system_clock::now())
                                .set_expires_at(chrono::system_clock::now() + chrono::days{15})
                                .sign(jwt::algorithm::hs256{l_ctx.secret_});
  }

  co_return in_handle->make_msg(l_ret);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(actions_persons_change_password, post) {
  person_.check_admin();
  auto& l_sql              = get_sqlite_database();
  auto l_person            = std::make_shared<person>(l_sql.get_by_uuid<person>(person_id_));
  auto l_json              = in_handle->get_json();
  const auto& l_password   = l_json.at("password").get_ref<const std::string&>();
  const auto& l_password_2 = l_json.at("password_2").get_ref<const std::string&>();
  DOODLE_CHICK(l_password == l_password_2, "两次输入密码不一致");
  l_person->password_ = bcrypt::generateHash(l_password);
  co_await l_sql.update(l_person);
  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}
DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(auth_change_password, post) {
  auto& l_sql                = get_sqlite_database();
  auto l_person              = std::make_shared<person>(l_sql.get_by_uuid<person>(person_.person_.uuid_id_));
  auto l_json                = in_handle->get_json();
  const auto& l_old_password = l_json.at("old_password").get_ref<const std::string&>();
  const auto& l_password     = l_json.at("password").get_ref<const std::string&>();
  const auto& l_password_2   = l_json.at("password_2").get_ref<const std::string&>();
  DOODLE_CHICK(bcrypt::validatePassword(l_old_password, l_person->password_), "旧密码错误");
  DOODLE_CHICK(l_password == l_password_2, "两次输入密码不一致");
  l_person->password_ = bcrypt::generateHash(l_password);
  co_await l_sql.update(l_person);
  co_return in_handle->make_msg(nlohmann::json{} = *l_person);
}
}  // namespace doodle::http
