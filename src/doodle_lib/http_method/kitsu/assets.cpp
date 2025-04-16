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
    j.at("data").get_to(p.data);
    j.at("description").get_to(p.description);
    j.at("is_shared").get_to(p.is_shared);
    j.at("name").get_to(p.name);
  }
};
}  // namespace
boost::asio::awaitable<boost::beast::http::message_generator> projects_assets_new_post::callback(
    session_data_ptr in_handle
) {
  get_person(in_handle);
  projects_assets_new_post_data l_data{};
  l_data.project_id = from_uuid_str(in_handle->capture_->get("project_id"));
  is_project_manager(l_data.project_id);
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
      .data_           = l_data.data,
      .created_by_     = person_->uuid_id_,
  });

  auto l_sql    = g_ctx().get<sqlite_database>();
  co_await l_sql.install(l_entity);
  co_return in_handle->make_msg((nlohmann::json{} = *l_entity).dump());
}

boost::asio::awaitable<boost::beast::http::message_generator> asset_details_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  auto l_asset_id = from_uuid_str(in_handle->capture_->get("asset_id"));
  auto&& l_sql    = g_ctx().get<sqlite_database>();
  auto l_t        = l_sql.get_assets_and_tasks(*person_, {}, l_asset_id);
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
  get_person(in_handle);
  uuid l_prj_id{};
  for (auto&& l_i : in_handle->url_.params())
    if (l_i.key == "project_id") l_prj_id = from_uuid_str(l_i.value);
  auto l_list = g_ctx().get<sqlite_database>().get_assets_and_tasks(*person_, l_prj_id);
  co_return in_handle->make_msg((nlohmann::json{} = l_list).dump());
}
boost::asio::awaitable<boost::beast::http::message_generator> shared_used_get::callback(session_data_ptr in_handle) {
  get_person(in_handle);
  co_return in_handle->make_msg("[]");
}
}  // namespace doodle::http

namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> assets_new(session_data_ptr in_handle) {
  if (in_handle->content_type_ != detail::content_type::application_json)
    co_return in_handle->make_error_code_msg(boost::beast::http::status::bad_request, "错误的请求类型");
  auto l_c     = create_kitsu_proxy(in_handle);

  auto& l_json = std::get<nlohmann::json>(in_handle->body_);
  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  l_request.body() = l_json.dump();
  l_request.prepare_payload();
  auto [l_ec, l_res] = co_await detail::read_and_write<boost::beast::http::string_body>(l_c, l_request);
  if (l_ec) {
    co_return in_handle->make_error_code_msg(boost::beast::http::status::internal_server_error, "服务器错误");
  }
  co_return std::move(l_res);
}
}  // namespace

void assets_reg2(http_route& in_http_route) {
  in_http_route.reg(
      std::make_shared<http_function>(
          boost::beast::http::verb::post, "api/data/projects/{project_id}/asset-types/{asset_type_id}/assets/new",
          assets_new
      )
  )

      ;
}
}  // namespace doodle::http::kitsu