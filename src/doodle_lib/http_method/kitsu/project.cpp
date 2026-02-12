//
// Created by TD on 24-11-7.
//

#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/entity.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/project.h>
#include <doodle_core/sqlite_orm/detail/sqlite_database_impl.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/core/socket_io/broadcast.h>
#include <doodle_lib/http_method/kitsu.h>

#include "kitsu_reg_url.h"
#include <algorithm>

namespace doodle::http {

namespace {
struct project_all_get_result_t : project {
  explicit project_all_get_result_t(const project& p, const std::string& in_status_name) : project(p) {
    project_status_name = in_status_name;
  }
  std::string project_status_name;
  // to json
  friend void to_json(nlohmann::json& j, const project_all_get_result_t& p) {
    to_json(j, static_cast<const project&>(p));
    j["project_status_name"] = p.project_status_name;
  }
};

auto select_project_all_get_result(const std::string& in_name) {
  auto l_sql = g_ctx().get<sqlite_database>();
  std::vector<project_all_get_result_t> l_list{};
  using namespace sqlite_orm;
  for (auto&& [l_prj, l_status_name] : l_sql.impl_->storage_any_.select(
           columns(object<project>(true), &project_status::name_), from<project>(),
           join<project_status>(on(c(&project::project_status_id_) == c(&project_status::uuid_id_))),
           where(in_name.empty() || c(&project::name_) == in_name), order_by(&project_status::name_)
       ))
    l_list.emplace_back(project_all_get_result_t(l_prj, l_status_name));
  return l_list;
}

}  // namespace

boost::asio::awaitable<boost::beast::http::message_generator> project_all::get(session_data_ptr in_handle) {
  person_.check_admin();
  co_return in_handle->make_msg(nlohmann::json{} = select_project_all_get_result({}));
}

boost::asio::awaitable<boost::beast::http::message_generator> data_project_instance::get(session_data_ptr in_handle) {
  auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<project>(id_);
  nlohmann::json l_j{};
  l_j = l_list;
  l_j["project_status_name"] =
      g_ctx().get<sqlite_database>().get_by_uuid<project_status>(l_list.project_status_id_).name_;
  co_return in_handle->make_msg(l_list);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_instance::put(session_data_ptr in_handle) {
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_project = std::make_shared<project>(l_sql.get_by_uuid<project>(id_));
  in_handle->get_json().get_to(*l_project);
  co_await l_sql.update(l_project);
  co_return in_handle->make_msg(nlohmann::json{} = *l_project);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_projects::post(session_data_ptr in_handle) {
  person_.check_manager();
  auto l_sql  = g_ctx().get<sqlite_database>();
  auto l_json = in_handle->get_json();
  auto l_prj  = std::make_shared<project>();
  l_json.get_to(*l_prj);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始创建项目 name {}", person_.person_.email_,
      person_.person_.get_full_name(), l_prj->name_
  );

  l_prj->project_status_id_ = l_sql.get_project_status_open();
  co_await g_ctx().get<sqlite_database>().install(l_prj);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成创建项目 project_id {} name {}", person_.person_.email_,
      person_.person_.get_full_name(), l_prj->uuid_id_, l_prj->name_
  );

  co_return in_handle->make_msg(nlohmann::json{} = *l_prj);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_settings_task_types::post(
    session_data_ptr in_handle
) {
  auto l_json = in_handle->get_json();
  person_.check_project_manager(id_);
  auto l_prj_task_type_link = std::make_shared<project_task_type_link>();
  l_json.get_to(*l_prj_task_type_link);
  l_prj_task_type_link->project_id_ = id_;

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始设置项目 {} 任务类型 task_type_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_prj_task_type_link->task_type_id_
  );

  if (auto l_t = g_ctx().get<sqlite_database>().get_project_task_type_link(id_, l_prj_task_type_link->task_type_id_);
      !l_t) {
    co_await g_ctx().get<sqlite_database>().install(l_prj_task_type_link);
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成设置项目 {} 任务类型 task_type_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_prj_task_type_link->task_type_id_
  );

  co_return in_handle->make_msg(nlohmann::json{} = g_ctx().get<sqlite_database>().get_by_uuid<project>(id_));
}
boost::asio::awaitable<boost::beast::http::message_generator> project_settings_task_types::delete_(
    session_data_ptr in_handle
) {
  person_.check_project_manager(project_id_);
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 删除 项目 {} 的任务类型 {} ", person_.person_.email_,
      person_.person_.get_full_name(), project_id_, task_type_id_
  );
  if (auto l_t = g_ctx().get<sqlite_database>().get_project_task_type_link(project_id_, task_type_id_); l_t)
    co_await g_ctx().get<sqlite_database>().remove<project_task_status_link>(l_t->uuid_id_);
  co_return in_handle->make_msg_204();
}

boost::asio::awaitable<boost::beast::http::message_generator> data_project_settings_task_status::post(
    session_data_ptr in_handle
) {
  auto l_json = in_handle->get_json();
  person_.check_project_manager(id_);
  auto l_status_id                        = l_json["task_status_id"].get<uuid>();
  auto l_prj_task_status_link             = std::make_shared<project_task_status_link>();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始设置项目 {} 任务状态 task_status_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_status_id
  );

  l_prj_task_status_link->project_id_     = id_;
  l_prj_task_status_link->task_status_id_ = l_status_id;
  if (!g_ctx().get<sqlite_database>().get_project_task_status_link(id_, l_status_id))
    co_await g_ctx().get<sqlite_database>().install(l_prj_task_status_link);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成设置项目 {} 任务状态 task_status_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_status_id
  );

  co_return in_handle->make_msg(nlohmann::json{} = g_ctx().get<sqlite_database>().get_by_uuid<project>(id_));
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_settings_asset_types::post(
    session_data_ptr in_handle
) {
  auto l_json = in_handle->get_json();
  person_.check_project_manager(id_);
  auto l_prj_asset_type_link            = std::make_shared<project_asset_type_link>();
  l_prj_asset_type_link->asset_type_id_ = l_json["asset_type_id"].get<uuid>();
  l_prj_asset_type_link->project_id_    = id_;

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始设置项目 {} 资产类型 asset_type_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_prj_asset_type_link->asset_type_id_
  );
  if (!g_ctx().get<sqlite_database>().get_project_asset_type_link(id_, l_prj_asset_type_link->asset_type_id_))
    co_await g_ctx().get<sqlite_database>().install(l_prj_asset_type_link);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成设置项目 {} 资产类型 asset_type_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_prj_asset_type_link->asset_type_id_
  );

  co_return in_handle->make_msg(nlohmann::json{} = g_ctx().get<sqlite_database>().get_by_uuid<project>(id_));
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_create_tasks::post(session_data_ptr in_handle) {
  person_.check_project_manager(project_id_);
  auto& l_sql      = g_ctx().get<sqlite_database>();
  auto l_task_type = l_sql.get_by_uuid<task_type>(task_type_id_);
  std::vector<entity> l_entities{};
  auto l_json = in_handle->get_json();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始在项目 {} 创建任务 task_type_id {}", person_.person_.email_,
      person_.person_.get_full_name(), project_id_, task_type_id_
  );
  if (l_json.is_array())
    for (auto&& l_v : l_json.get<std::vector<uuid>>()) l_entities.emplace_back(l_sql.get_by_uuid<entity>(l_v));
  else
    for (auto&& [l_k, l_v, l_has] : in_handle->url_.params())
      if (l_k == "id" && l_has) l_entities.emplace_back(l_sql.get_by_uuid<entity>(from_uuid_str(l_v)));
  auto l_task_status = l_sql.get_task_status_by_name(std::string{doodle_config::task_status_todo});

  {  // 添加类型检查
    auto l_asset_types = l_sql.get_asset_types_not_temporal_type();
    for (auto&& l_e : l_entities)
      if (auto l_it = std::ranges::find_if(
              l_asset_types, [&](const asset_type& in) -> bool { return in.uuid_id_ == l_e.entity_type_id_; }
          );
          l_it != l_asset_types.end()) {
        if (auto l_it2 =
                std::ranges::find_if(l_it->task_types_, [&](const uuid& in) { return in == l_task_type.uuid_id_; });
            l_it2 == l_it->task_types_.end())
          throw_exception(
              http_request_error{
                  boost::beast::http::status::bad_request, fmt::format("实体类型不在工作流中 {}", l_task_type.name_)
              }
          );
      } else
        throw_exception(
            http_request_error{
                boost::beast::http::status::bad_request, fmt::format("实体类型未找到 {}", l_e.entity_type_id_)
            }
        );
  }

  if (l_entities.size() == 1) {
    auto l_task             = std::make_shared<task>();
    l_task->name_           = "main";
    l_task->project_id_     = project_id_;
    l_task->task_type_id_   = l_task_type.uuid_id_;
    l_task->entity_id_      = l_entities[0].uuid_id_;
    l_task->task_status_id_ = l_task_status.uuid_id_;
    co_await l_sql.install(l_task);

    nlohmann::json l_json_r{};
    auto&& l_task_json                    = l_json_r.emplace_back(*l_task);
    l_task_json["assignees"]              = nlohmann::json::array();
    l_task_json["task_status_id"]         = l_task_status.uuid_id_;
    l_task_json["task_status_name"]       = l_task_status.name_;
    l_task_json["task_status_short_name"] = l_task_status.short_name_;
    l_task_json["task_status_color"]      = l_task_status.color_;
    l_task_json["task_type_id"]           = l_task_type.uuid_id_;
    l_task_json["task_type_name"]         = l_task_type.name_;
    l_task_json["task_type_color"]        = l_task_type.color_;
    l_task_json["task_type_priority"]     = l_task_type.priority_;

    socket_io::broadcast(
        "task:new", nlohmann::json{{"task_id", l_task->uuid_id_}, {"project_id", l_task->project_id_}}, "/events"
    );

    SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成在项目 {} 创建任务 task_id {} entity_id {} task_type_id {}",
      person_.person_.email_, person_.person_.get_full_name(), project_id_, l_task->uuid_id_, l_task->entity_id_,
      l_task->task_type_id_
    );

    co_return in_handle->make_msg(l_json_r.dump());
  }
  auto l_tasks = std::make_shared<std::vector<task>>();
  for (auto&& i : l_entities) {
    if (l_sql.is_task_exist(i.uuid_id_, l_task_type.uuid_id_)) continue;
    l_tasks->emplace_back(
        task{
            .name_           = "main",
            .project_id_     = i.project_id_,
            .task_type_id_   = l_task_type.uuid_id_,
            .task_status_id_ = l_task_status.uuid_id_,
            .entity_id_      = i.uuid_id_,

        }
    );
  }
  co_await l_sql.install_range(l_tasks);
  nlohmann::json l_json_r{};
  l_json_r = *l_tasks;
  for (auto&& i : l_json_r) {
    i["assignees"]              = nlohmann::json::array();
    i["task_status_id"]         = l_task_status.uuid_id_;
    i["task_status_name"]       = l_task_status.name_;
    i["task_status_short_name"] = l_task_status.short_name_;
    i["task_status_color"]      = l_task_status.color_;
    i["task_type_id"]           = l_task_type.uuid_id_;
    i["task_type_name"]         = l_task_type.name_;
    i["task_type_color"]        = l_task_type.color_;
    i["task_type_priority"]     = l_task_type.priority_;
  }
  for (const auto& i : *l_tasks)
    socket_io::broadcast("task:new", nlohmann::json{{"task_id", i.uuid_id_}, {"project_id", i.project_id_}}, "/events");

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成在项目 {} 批量创建任务 task_type_id {} 数量 {}", person_.person_.email_,
      person_.person_.get_full_name(), project_id_, task_type_id_, l_tasks->size()
  );

  co_return in_handle->make_msg(l_json_r);
}

boost::asio::awaitable<boost::beast::http::message_generator> data_projects_team::post(session_data_ptr in_handle) {
  person_.check_project_manager(id_);
  auto l_add_team = in_handle->get_json()["person_id"].get<uuid>();
  auto l_sql      = g_ctx().get<sqlite_database>();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始向项目 {} 添加成员 {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_add_team
  );

  using namespace sqlite_orm;
  if (!l_sql.is_person_in_project(l_add_team, id_)) {
    auto l_team         = std::make_shared<project_person_link>();
    l_team->project_id_ = id_;
    l_team->person_id_  = l_add_team;
    co_await l_sql.install(l_team);
    socket_io::broadcast("project:update", nlohmann::json{{"project_id", id_}}, "/events");
  }
  auto l_prj = l_sql.get_by_uuid<project>(id_);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成向项目 {} 添加成员 {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_add_team
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_prj);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_team_person::delete_(
    session_data_ptr in_handle
) {
  person_.check_project_manager(project_id_);
  auto l_sql = g_ctx().get<sqlite_database>();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 从项目 {} 移除成员 {}", person_.person_.email_,
      person_.person_.get_full_name(), project_id_, person_id_
  );

  using namespace sqlite_orm;
  if (auto l_id = l_sql.impl_->storage_any_.select(
          &project_person_link::id_,
          where(
              c(&project_person_link::project_id_) == project_id_ && c(&project_person_link::person_id_) == person_id_
          )
      );
      !l_id.empty()) {
    co_await l_sql.remove<project_person_link>(l_id[0]);
    socket_io::broadcast("project:update", nlohmann::json{{"project_id", project_id_}}, "/events");
  }
  co_return in_handle->make_msg_204();
}
boost::asio::awaitable<boost::beast::http::message_generator> data_task_type_links::post(session_data_ptr in_handle) {
  auto l_args = in_handle->get_json().get<project_task_type_link>();
  person_.check_project_manager(l_args.project_id_);
  auto l_sql = g_ctx().get<sqlite_database>();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始设置项目 {} 任务类型关联 task_type_id {} priority {}",
      person_.person_.email_, person_.person_.get_full_name(), l_args.project_id_, l_args.task_type_id_,
      l_args.priority_.value_or(0)
  );

  auto l_ptr = std::make_shared<project_task_type_link>(
      l_sql.get_project_task_type_link(l_args.project_id_, l_args.task_type_id_).value_or(l_args)
  );
  l_ptr->priority_ = l_args.priority_.value_or(0);
  if (!l_ptr->id_)
    co_await l_sql.install(l_ptr);
  else
    co_await l_sql.update(l_ptr);

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成设置项目 {} 任务类型关联 id {} task_type_id {} priority {}",
      person_.person_.email_, person_.person_.get_full_name(), l_ptr->project_id_, l_ptr->id_, l_ptr->task_type_id_,
      l_ptr->priority_
  );

  co_return in_handle->make_msg(nlohmann::json{} = *l_ptr);
}
boost::asio::awaitable<boost::beast::http::message_generator> data_project_settings_status_automations::post(
    session_data_ptr in_handle
) {
  person_.check_project_manager(id_);
  auto l_sql                   = g_ctx().get<sqlite_database>();
  auto l_ptr                   = std::make_shared<project_status_automation_link>();
  l_ptr->project_id_           = id_;
  l_ptr->status_automation_id_ = in_handle->get_json().at("status_automation_id").get<uuid>();

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 开始设置项目 {} 状态自动化 status_automation_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_ptr->status_automation_id_
  );

  auto l_prj                   = l_sql.get_by_uuid<project>(id_);
  if (std::ranges::find(l_prj.status_automations_, l_ptr->status_automation_id_) == l_prj.status_automations_.end()) {
    co_await l_sql.install(l_ptr);
    l_prj.status_automations_.push_back(l_ptr->status_automation_id_);
  }

  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 完成设置项目 {} 状态自动化 status_automation_id {}", person_.person_.email_,
      person_.person_.get_full_name(), id_, l_ptr->status_automation_id_
  );

  co_return in_handle->make_msg(nlohmann::json{} = l_prj);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(data_project_settings_status_automations_instance, delete_) {
  person_.check_project_manager(project_id_);
  auto l_sql = g_ctx().get<sqlite_database>();
  SPDLOG_LOGGER_WARN(
      g_logger_ctrl().get_http(), "用户 {}({}) 从项目 {} 移除状态自动化 {}", person_.person_.email_,
      person_.person_.get_full_name(), project_id_, status_automation_id_
  );
  using namespace sqlite_orm;
  if (auto l_id = l_sql.impl_->storage_any_.select(
          &project_status_automation_link::id_,
          where(
              c(&project_status_automation_link::project_id_) == project_id_ &&
              c(&project_status_automation_link::status_automation_id_) == status_automation_id_
          )
      );
      !l_id.empty()) {
    co_await l_sql.remove<project_status_automation_link>(l_id.front());
  }
  auto l_prj = l_sql.get_by_uuid<project>(project_id_);
  co_return in_handle->make_msg(nlohmann::json{} = l_prj);
}
}  // namespace doodle::http
