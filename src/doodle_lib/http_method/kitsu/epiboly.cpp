//
// Created by TD on 24-12-13.
//

#include "epiboly.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/user.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_function.h>
#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_client/dingding_client.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> config(session_data_ptr in_handle) {
  co_return in_handle->make_msg(R"(
{
    "is_self_hosted": true,
    "crisp_token": "",
    "dark_theme_by_default": null,
    "indexer_configured": true,
    "saml_enabled": false,
    "saml_idp_name": "",
    "default_locale": "zh",
    "default_timezone": "Asia/Shanghai"
}
)"s);
}
boost::asio::awaitable<boost::beast::http::message_generator> authenticated(session_data_ptr in_handle) {
  co_return in_handle->make_error_code_msg(boost::beast::http::status::unauthorized, R"({
    "msg": "Missing JWT in cookies or headers Missing cookie \"access_token_cookie\"; Missing Authorization Header"
})"s);
}

boost::asio::awaitable<boost::beast::http::message_generator> user_context(session_data_ptr in_handle) {
  nlohmann::json l_json{};
  l_json["projects"] = nlohmann::json::parse(R"([{
  "asset_types": [],
  "auto_upload_path": "//192.168.10.240/public/后期/JJ_DJ/",
  "code": "JJ",
  "created_at": "2024-11-07T05:40:42",
  "data": null,
  "default_preview_background_file_id": null,
  "description": null,
  "descriptors": [],
  "en_str": "JingJie",
  "end_date": "2027-11-30",
  "episode_span": 0,
  "file_tree": null,
  "fps": "25",
  "has_avatar": false,
  "hd_bitrate_compression": null,
  "homepage": "assets",
  "id": "d41d3dae-f86b-4080-850d-3d41ef272dd2",
  "is_clients_isolated": false,
  "is_preview_download_allowed": false,
  "is_publish_default_for_artists": false,
  "is_set_preview_automated": false,
  "ld_bitrate_compression": null,
  "man_days": null,
  "max_retakes": 0,
  "name": "镜·界",
  "nb_episodes": 0,
  "path": "//192.168.10.242/public/JJ_DJ",
  "preview_background_files": [],
  "production_style": "3d",
  "production_type": "featurefilm",
  "project_status_id": "755c9edd-9481-4145-ab43-21491bdf2739",
  "ratio": "9:16",
  "resolution": "1080x1920",
  "shotgun_id": null,
  "start_date": "2024-09-01",
  "status_automations": [],
  "task_statuses": [],
  "task_statuses_link": {},
  "task_types": [],
  "task_types_priority": {},
  "team": [],
  "type": "Project",
  "updated_at": "2024-12-26T03:47:56"
}])");
  co_return in_handle->make_msg(l_json.dump());
}
}  // namespace
void epiboly_reg(http_route& in_http_route) {
  in_http_route.reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/config", config))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/auth/authenticated", authenticated))
      .reg(std::make_shared<http_function>(boost::beast::http::verb::get, "api/data/user/context", user_context));
}
}  // namespace doodle::http::kitsu