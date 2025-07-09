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
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>

#include "kitsu.h"
#include "kitsu_reg_url.h"

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

boost::asio::awaitable<boost::beast::http::message_generator> project_all_get::callback_arg(
    session_data_ptr in_handle
) {
  auto l_ptr = get_person(in_handle);
  l_ptr->is_admin();
  co_return in_handle->make_msg(nlohmann::json{} = select_project_all_get_result({}));
}

boost::asio::awaitable<boost::beast::http::message_generator> project_get::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_ptr  = get_person(in_handle);
  auto l_list = g_ctx().get<sqlite_database>().get_by_uuid<project>(in_arg->id_);
  nlohmann::json l_j{l_list};
  l_j["project_status_name"] =
      g_ctx().get<sqlite_database>().get_by_uuid<project_status>(l_list.project_status_id_).name_;
  co_return in_handle->make_msg(nlohmann::json{l_list});
}
boost::asio::awaitable<boost::beast::http::message_generator> project_put::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_ptr     = get_person(in_handle);
  auto l_sql     = g_ctx().get<sqlite_database>();
  auto l_project = std::make_shared<project>(l_sql.get_by_uuid<project>(in_arg->id_));
  in_handle->get_json().get_to(*l_project);
  co_await l_sql.install(l_project);
  co_return in_handle->make_msg(nlohmann::json{} = *l_project);
}

boost::asio::awaitable<boost::beast::http::message_generator> project_c_post::callback_arg(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);
  l_ptr->is_manager();

  auto l_json = in_handle->get_json();
  auto l_prj  = std::make_shared<project>();
  l_json.get_to(*l_prj);
  l_prj->uuid_id_ = core_set::get_set().get_uuid();
  co_await g_ctx().get<sqlite_database>().install(l_prj);
  co_return in_handle->make_msg(nlohmann::json{*l_prj}.dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> project_settings_task_types_post::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_ptr  = get_person(in_handle);
  auto l_json = in_handle->get_json();
  l_ptr->is_project_manager(in_arg->id_);
  auto l_prj_task_type_link = std::make_shared<project_task_type_link>();
  l_json.get_to(*l_prj_task_type_link);
  l_prj_task_type_link->project_id_ = in_arg->id_;
  if (l_prj_task_type_link->uuid_id_.is_nil()) l_prj_task_type_link->uuid_id_ = core_set::get_set().get_uuid();

  if (auto l_t =
          g_ctx().get<sqlite_database>().get_project_task_type_link(in_arg->id_, l_prj_task_type_link->task_type_id_);
      !l_t) {
    co_await g_ctx().get<sqlite_database>().install(l_prj_task_type_link);
  }
  co_return in_handle->make_msg(nlohmann::json{g_ctx().get<sqlite_database>().get_by_uuid<project>(in_arg->id_)});
}
boost::asio::awaitable<boost::beast::http::message_generator> project_settings_task_types_delete_::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<project_settings_task_types_arg> in_arg
) {
  auto l_ptr = get_person(in_handle);
  l_ptr->is_project_manager(in_arg->project_id_);
  if (auto l_t = g_ctx().get<sqlite_database>().get_project_task_type_link(in_arg->project_id_, in_arg->task_type_id_);
      l_t)
    co_await g_ctx().get<sqlite_database>().remove<project_task_status_link>(l_t->uuid_id_);
  co_return in_handle->make_msg_204();
}

boost::asio::awaitable<boost::beast::http::message_generator> project_settings_task_status_post::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_ptr  = get_person(in_handle);
  auto l_json = in_handle->get_json();
  l_ptr->is_project_manager(in_arg->id_);
  auto l_status_id                        = l_json["task_status_id"].get<uuid>();
  auto l_prj_task_status_link             = std::make_shared<project_task_status_link>();
  l_prj_task_status_link->project_id_     = in_arg->id_;
  l_prj_task_status_link->task_status_id_ = l_status_id;
  if (!g_ctx().get<sqlite_database>().get_project_task_status_link(in_arg->id_, l_status_id))
    co_await g_ctx().get<sqlite_database>().install(l_prj_task_status_link);
  co_return in_handle->make_msg(nlohmann::json{g_ctx().get<sqlite_database>().get_by_uuid<project>(in_arg->id_)});
}
boost::asio::awaitable<boost::beast::http::message_generator> project_settings_asset_types_post::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<capture_id_t> in_arg
) {
  auto l_ptr  = get_person(in_handle);
  auto l_json = in_handle->get_json();
  l_ptr->is_project_manager(in_arg->id_);
  auto l_prj_asset_type_link            = std::make_shared<project_asset_type_link>();
  l_prj_asset_type_link->asset_type_id_ = l_json["asset_type_id"].get<uuid>();
  l_prj_asset_type_link->project_id_    = in_arg->id_;
  if (!g_ctx().get<sqlite_database>().get_project_asset_type_link(in_arg->id_, l_prj_asset_type_link->asset_type_id_))
    co_await g_ctx().get<sqlite_database>().install(l_prj_asset_type_link);
  co_return in_handle->make_msg(nlohmann::json{g_ctx().get<sqlite_database>().get_by_uuid<project>(in_arg->id_)});
}
boost::asio::awaitable<boost::beast::http::message_generator> actions_create_tasks_post::callback_arg(
    session_data_ptr in_handle, std::shared_ptr<actions_create_tasks_arg> in_arg
) {
  auto l_ptr       = get_person(in_handle);
  auto& l_sql      = g_ctx().get<sqlite_database>();
  auto l_task_type = l_sql.get_by_uuid<task_type>(in_arg->task_type_id);
  std::vector<entity> l_entities{};
  auto l_json = in_handle->get_json();
  if (l_json.is_array())
    for (auto&& l_v : l_json.get<std::vector<uuid>>()) l_entities.emplace_back(l_sql.get_by_uuid<entity>(l_v));
  else
    for (auto&& [l_k, l_v, l_has] : in_handle->url_.params())
      if (l_k == "id" && l_has) l_entities.emplace_back(l_sql.get_by_uuid<entity>(from_uuid_str(l_v)));
  auto l_task_status = l_sql.get_task_status_by_name(std::string{doodle_config::task_status_todo});

  if (l_entities.size() == 1) {
    auto l_task             = std::make_shared<task>();
    l_task->uuid_id_        = core_set::get_set().get_uuid();
    l_task->name_           = "main";
    l_task->project_id_     = in_arg->project_id;
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

    co_return in_handle->make_msg(l_json_r.dump());
  }
  auto l_tasks = std::make_shared<std::vector<task>>();
  for (auto&& i : l_entities) {
    if (l_sql.is_task_exist(i.uuid_id_, l_task_type.uuid_id_)) continue;
    l_tasks->emplace_back(
        task{
            .uuid_id_        = core_set::get_set().get_uuid(),
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
  co_return in_handle->make_msg(l_json_r.dump());
}

}  // namespace doodle::http
