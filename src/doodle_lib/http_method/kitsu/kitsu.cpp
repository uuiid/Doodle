//
// Created by TD on 24-8-21.
//
#include "kitsu.h"

#include <doodle_core/core/app_base.h>
#include <doodle_core/core/authorization.h>
#include <doodle_core/metadata/assets_file.h>
#include <doodle_core/metadata/entity_type.h>
#include <doodle_core/metadata/kitsu/assets_type.h>
#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/metadata/project_status.h>
#include <doodle_core/metadata/task_status.h>
#include <doodle_core/metadata/task_type.h>
#include <doodle_core/platform/win/register_file_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/cache_manger.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computer.h>
#include <doodle_lib/http_method/file_association.h>
#include <doodle_lib/http_method/kitsu/assets.h>
#include <doodle_lib/http_method/kitsu/epiboly.h>
#include <doodle_lib/http_method/kitsu/http_route_proxy.h>
#include <doodle_lib/http_method/kitsu/kitsu_front_end.h>
#include <doodle_lib/http_method/kitsu/project.h>
#include <doodle_lib/http_method/kitsu/task.h>
#include <doodle_lib/http_method/kitsu/user.h>
#include <doodle_lib/http_method/kitsu_front_end_reg.h>
#include <doodle_lib/http_method/local/event.h>
#include <doodle_lib/http_method/local/local_setting.h>
#include <doodle_lib/http_method/local/task_info.h>
#include <doodle_lib/http_method/model_library/assets.h>
#include <doodle_lib/http_method/model_library/assets_tree.h>
#include <doodle_lib/http_method/model_library/thumbnail.h>
#include <doodle_lib/http_method/tool_version.h>
#include <doodle_lib/http_method/up_file.h>
namespace doodle::http {

http_route_ptr create_kitsu_route(const FSys::path& in_root) {
  auto l_router = std::make_shared<kitsu::http_route_proxy>();
  l_router->reg_proxy(std::make_shared<doodle::kitsu::kitsu_proxy_url>("api"))
      .reg_proxy(std::make_shared<doodle::kitsu::kitsu_proxy_url>("socket.io"));
  kitsu::user_reg(*l_router);
  kitsu::task_reg(*l_router);
  kitsu::assets_reg(*l_router);
  kitsu::assets_tree_reg(*l_router);
  kitsu::thumbnail_reg(*l_router);
  kitsu::project_reg(*l_router);
  kitsu::assets_reg2(*l_router);
  up_file_reg(*l_router);
  reg_file_association_http(*l_router);
  reg_kitsu_front_end_http(*l_router, in_root);
  computer_reg(*l_router);
  tool_version_reg(*l_router);
  g_ctx().emplace<cache_manger>();
  return l_router;
}

http_route_ptr create_kitsu_local_route() {
  auto l_rout_ptr = std::make_shared<http::http_route>();
  http::local_setting_reg(*l_rout_ptr);
  local::local_event_reg(*l_rout_ptr);
  if (g_ctx().get<authorization>().is_expire()) http::task_info_reg_local(*l_rout_ptr);
  return l_rout_ptr;
}

http_route_ptr create_kitsu_epiboly_route(const FSys::path& in_root) {
  auto l_router = std::make_shared<kitsu::http_route_proxy>();
  l_router->reg_proxy(std::make_shared<doodle::kitsu::kitsu_proxy_url>("api"))
      .reg_proxy(std::make_shared<doodle::kitsu::kitsu_proxy_url>("socket.io"));
  reg_kitsu_front_end_http(*l_router, in_root);
  kitsu::epiboly_reg(*l_router);
  tool_version_reg(*l_router);
  return l_router;
}

namespace kitsu {
namespace {

boost::asio::awaitable<void> init_context_impl() {
  auto& l_data = g_ctx().get<sqlite_database>();
  if (l_data.get_all<project_status>().size() == 0) {
    auto l_s      = std::make_shared<project_status>();
    l_s->uuid_id_ = from_uuid_str("755c9edd-9481-4145-ab43-21491bdf2739");
    l_s->name_    = "Open";
    l_s->color_   = "#000000";
    co_await l_data.install(l_s);
    l_s           = std::make_shared<project_status>();
    l_s->uuid_id_ = from_uuid_str("5159f210-7ec8-40e3-b8c9-2a06d0b4b116");
    l_s->name_    = "Closed";
    l_s->color_   = "#000000";
    co_await l_data.install(l_s);
  }
  if (l_data.get_all<task_status>().size() == 0) {
    boost::beast::http::request<boost::beast::http::empty_body> l_req{
        boost::beast::http::verb::get, "/api/data/task-status", 11
    };
    auto l_r         = co_await g_ctx().get<std::shared_ptr<doodle::kitsu::kitsu_client>>()->get(std::move(l_req));
    auto l_task_list = std::make_shared<std::vector<task_status>>();

    for (auto& l_j : *l_r) {
      auto l_task     = l_j.get<task_status>();
      l_task.uuid_id_ = l_j["id"].get<uuid>();
      l_task_list->emplace_back(l_task);
    }

    co_await l_data.install_range(l_task_list);
  }
  if (l_data.get_all<task_type>().size() == 0) {
    boost::beast::http::request<boost::beast::http::empty_body> l_req{
        boost::beast::http::verb::get, "/api/data/task-types", 11
    };
    auto l_r         = co_await g_ctx().get<std::shared_ptr<doodle::kitsu::kitsu_client>>()->get(std::move(l_req));
    auto l_task_list = std::make_shared<std::vector<task_type>>();
    for (auto& l_j : *l_r) {
      auto l_task     = l_j.get<task_type>();
      l_task.uuid_id_ = l_j["id"].get<uuid>();
      l_task_list->emplace_back(l_task);
    }
    co_await l_data.install_range(l_task_list);
  }
  if (l_data.get_all<asset_type>().size() == 0) {
    boost::beast::http::request<boost::beast::http::empty_body> l_req{
        boost::beast::http::verb::get, "/api/data/asset-types", 11
    };
    auto l_r = co_await g_ctx().get<std::shared_ptr<doodle::kitsu::kitsu_client>>()->get(std::move(l_req));
    for (auto& l_j : *l_r) {
      auto l_task_list      = std::make_shared<asset_type>();
      *l_task_list          = l_j.get<asset_type>();
      l_task_list->uuid_id_ = l_j["id"].get<uuid>();
      co_await l_data.install(l_task_list);
      for (auto&& i : l_task_list->task_types_) {
        auto l_link            = std::make_shared<task_type_asset_type_link>();
        l_link->asset_type_id_ = l_task_list->uuid_id_;
        l_link->task_type_id_  = i;
        co_await l_data.install(l_link);
      }
    }
  }
  app_base::Get().stop_app();
}
}  // namespace
void init_context() { boost::asio::co_spawn(g_strand(), init_context_impl(), boost::asio::detached); }

http::detail::http_client_data_base_ptr create_kitsu_proxy(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data{};
  if (!in_handle->user_data_.has_value()) {
    kitsu_data_t l_data{std::make_shared<detail::http_client_data_base>(g_io_context().get_executor())};
    l_client_data = l_data.http_kitsu_;
    l_client_data->init(g_ctx().get<kitsu_ctx_t>().url_);
    in_handle->user_data_ = l_data;
  } else {
    l_client_data = std::any_cast<kitsu_data_t&>(in_handle->user_data_).http_kitsu_;
  }
  return l_client_data;
}

project_helper::database_t find_project(const std::string& in_name) {
  auto l_prj = g_ctx().get<sqlite_database>().find_project_by_name(in_name);
  if (l_prj.empty()) return {};
  return l_prj.front();
}
uuid get_url_project_id(const boost::urls::url& in_url) {
  auto l_q = in_url.query();
  if (auto l_it = l_q.find("project_id"); l_it != l_q.npos) {
    auto l_str = l_q.substr(l_it + 11, l_q.find('&', l_it) - l_it - 11);
    return boost::lexical_cast<uuid>(l_str);
  }
  return {};
}
doodle::details::assets_type_enum conv_assets_type_enum(const std::string& in_name) {
  if (in_name == "角色") {
    return doodle::details::assets_type_enum::character;
  } else if (in_name == "场景") {
    return doodle::details::assets_type_enum::scene;
  } else if (in_name == "道具") {
    return doodle::details::assets_type_enum::prop;
  } else if (in_name == "绑定") {
    return doodle::details::assets_type_enum::rig;
  } else if (in_name == "动画") {
    return doodle::details::assets_type_enum::animation;
  } else if (in_name == "特效") {
    return doodle::details::assets_type_enum::vfx;
  } else if (in_name == "地编模型") {
    return doodle::details::assets_type_enum::scene;
  } else if (in_name == "地编") {
    return doodle::details::assets_type_enum::scene;
  }
  return doodle::details::assets_type_enum::other;
}

std::string_view mime_type(const FSys::path& in_ext) {
  if (in_ext == ".htm") return "text/html";
  if (in_ext == ".html") return "text/html";
  if (in_ext == ".php") return "text/html";
  if (in_ext == ".css") return "text/css";
  if (in_ext == ".txt") return "text/plain";
  if (in_ext == ".js") return "application/javascript";
  if (in_ext == ".json") return "application/json";
  if (in_ext == ".xml") return "application/xml";
  if (in_ext == ".swf") return "application/x-shockwave-flash";
  if (in_ext == ".flv") return "video/x-flv";
  if (in_ext == ".png") return "image/png";
  if (in_ext == ".jpe") return "image/jpeg";
  if (in_ext == ".jpeg") return "image/jpeg";
  if (in_ext == ".jpg") return "image/jpeg";
  if (in_ext == ".gif") return "image/gif";
  if (in_ext == ".bmp") return "image/bmp";
  if (in_ext == ".ico") return "image/vnd.microsoft.icon";
  if (in_ext == ".tiff") return "image/tiff";
  if (in_ext == ".tif") return "image/tiff";
  if (in_ext == ".svg") return "image/svg+xml";
  if (in_ext == ".svgz") return "image/svg+xml";
  if (in_ext == ".map") return "application/json";
  if (in_ext == ".exe") return "application/octet-stream";
  if (in_ext == "m3u8") return "application/vnd.apple.mpegurl";
  if (in_ext == ".mp4") return "video/mp4";
  if (in_ext == ".webm") return "video/webm";
  if (in_ext == ".mkv") return "video/x-matroska";
  if (in_ext == ".ts") return "video/mp2t";
  if (in_ext == ".pdf") return "application/pdf";
  if (in_ext == ".zip") return "application/zip";
  if (in_ext == ".rar") return "application/x-rar-compressed";
  if (in_ext == ".7z") return "application/x-7z-compressed";
  if (in_ext == ".tar") return "application/x-tar";
  if (in_ext == ".log") return "text/plain";
  return "application/octet-stream";
}

}  // namespace kitsu

}  // namespace doodle::http