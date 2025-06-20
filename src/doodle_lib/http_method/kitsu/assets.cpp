//
// Created by TD on 24-12-30.
//

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>
#include <doodle_core/sqlite_orm/sqlite_select_data.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/http_jwt_fun.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
#include <doodle_lib/http_method/kitsu/kitsu_reg_url.h>

#include "kitsu.h"

namespace doodle::http {
namespace {
struct projects_assets_new_post_data {
  nlohmann::json data;
  std::string description;
  bool is_shared;
  std::string name;
  uuid source_id;

  uuid project_id;
  uuid asset_type_id;

  // form json
  friend void from_json(const nlohmann::json& j, projects_assets_new_post_data& p) {
    j.at("description").get_to(p.description);
    j.at("is_shared").get_to(p.is_shared);
    j.at("name").get_to(p.name);
  }
};
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> projects_assets_new_post::callback(
    session_data_ptr in_handle
) {
  auto l_ptr = get_person(in_handle);
  projects_assets_new_post_data l_data{};
  l_data.project_id = from_uuid_str(in_handle->capture_->get("project_id"));
  l_ptr->is_project_manager(l_data.project_id);
  l_data.asset_type_id = from_uuid_str(in_handle->capture_->get("asset_type_id"));
  in_handle->get_json().get_to(l_data);

  auto l_entity = std::make_shared<entity>(entity{
      .uuid_id_        = core_set::get_set().get_uuid(),
      .name_           = l_data.name,
      .description_    = l_data.description,
      .is_shared_      = l_data.is_shared,
      .project_id_     = l_data.project_id,
      .entity_type_id_ = l_data.asset_type_id,
      .source_id_      = l_data.source_id,
      .created_by_     = l_ptr->person_.uuid_id_,
  });

  auto l_sql    = g_ctx().get<sqlite_database>();
  co_await l_sql.install(l_entity);
  co_return in_handle->make_msg((nlohmann::json{} = *l_entity).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> asset_details_get::callback(session_data_ptr in_handle) {
  auto l_ptr      = get_person(in_handle);
  auto l_asset_id = from_uuid_str(in_handle->capture_->get("asset_id"));
  auto&& l_sql    = g_ctx().get<sqlite_database>();
  auto l_t        = l_sql.get_assets_and_tasks(l_ptr->person_, {}, l_asset_id);
  if (l_t.empty())
    throw_exception(
        http_request_error{boost::beast::http::status::not_found, fmt::format("未找到资源 {}", l_asset_id)}
    );
  auto l_ass                = l_sql.get_by_uuid<entity>(l_asset_id);
  auto l_project            = l_sql.get_by_uuid<project>(l_ass.project_id_);
  auto l_ass_type           = l_sql.get_by_uuid<asset_type>(l_ass.entity_type_id_);

  auto l_json               = nlohmann::json{};
  l_json                    = l_ass;
  l_json["project_name"]    = l_project.name_;
  l_json["asset_type_id"]   = l_ass_type.uuid_id_;
  l_json["asset_type_name"] = l_ass_type.name_;
  l_json.update(l_t[0]);
  co_return in_handle->make_msg(l_json.dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> with_tasks_get::callback(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);
  uuid l_prj_id{};
  for (auto&& l_i : in_handle->url_.params())
    if (l_i.key == "project_id") l_prj_id = from_uuid_str(l_i.value);
  auto l_list = g_ctx().get<sqlite_database>().get_assets_and_tasks(l_ptr->person_, l_prj_id);
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> shared_used_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  co_return in_handle->make_msg(nlohmann::json::array());
}

boost::asio::awaitable<boost::beast::http::message_generator> data_asset_delete_::callback(session_data_ptr in_handle) {
  auto l_ptr = get_person(in_handle);
  auto l_sql = g_ctx().get<sqlite_database>();
  auto l_ass = std::make_shared<entity>(l_sql.get_by_uuid<entity>(in_handle->capture_->get_uuid()));
  l_ptr->check_delete_access(l_ass->project_id_);
  bool l_force{};
  for (auto&& l_i : in_handle->url_.params()) {
    if (l_i.key == "force") l_force = true;
  }
  if (!l_force) {
    l_ass->canceled_ = true;
    co_await l_sql.install(l_ass);
    co_return in_handle->make_msg(nlohmann::json{} = *l_ass);
  }
  auto l_task     = l_sql.get_tasks_for_entity(l_ass->uuid_id_);
  auto l_task_ids = l_task | ranges::views::transform([](const task& in) { return in.uuid_id_; }) | ranges::to_vector;
  co_await l_sql.remove<task>(l_task_ids);
  co_await l_sql.remove<entity>(l_ass->uuid_id_);
  co_return in_handle->make_msg(nlohmann::json{} = *l_ass);
}

}  // namespace doodle::http
