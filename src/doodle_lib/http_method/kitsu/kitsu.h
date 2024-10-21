//
// Created by TD on 24-8-20.
//

#pragma once
#include <doodle_core/core/http_client_core.h>

#include <doodle_lib/core/http/http_session_data.h>
#include <doodle_lib/core/scan_assets/base.h>
namespace doodle::http {
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;

struct kitsu_data_t {
  std::shared_ptr<detail::http_client_data_base> http_kitsu_;
  std::map<boost::uuids::uuid, std::string> task_types_;
};

struct kitsu_ctx_t {
  std::string url_;
  std::string access_token_;
  std::map<boost::uuids::uuid, std::string> task_types_;
};

http_route_ptr create_kitsu_route(const FSys::path& in_root);

namespace kitsu {
http::detail::http_client_data_base_ptr create_kitsu_proxy(session_data_ptr in_handle);

project_helper::database_t find_project(const std::string& in_name);
doodle::details::assets_type_enum conv_assets_type_enum(const std::string& in_name);

uuid get_url_project_id(const boost::urls::url& in_url);

/**
 * @brief 初始化上下文, 项目, 任务类别
 */
void init_context();

std::string_view mime_type(const FSys::path& in_ext);
}  // namespace kitsu

}  // namespace doodle::http