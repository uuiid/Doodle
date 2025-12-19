//
// Created by TD on 24-8-20.
//

#pragma once
#include <doodle_core/core/http_client_core.h>
#include <doodle_core/metadata/assets.h>
#include <doodle_core/metadata/kitsu_ctx_t.h>

#include <doodle_lib/core/http/http_session_data.h>

namespace doodle::http {
class http_route;
using http_route_ptr = std::shared_ptr<http_route>;

http_route_ptr create_kitsu_route_2(const FSys::path& in_root);
http_route_ptr create_kitsu_epiboly_route(const FSys::path& in_root);
http_route_ptr create_kitsu_local_route();

namespace kitsu {

doodle::details::assets_type_enum conv_assets_type_enum(const std::string& in_name);

uuid get_url_project_id(const boost::urls::url& in_url);

std::string_view mime_type(const FSys::path& in_ext);

}  // namespace kitsu
void preview_reg(http_route& in_http_route);

}  // namespace doodle::http