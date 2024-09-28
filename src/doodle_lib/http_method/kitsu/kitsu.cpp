//
// Created by TD on 24-8-21.
//
#include "kitsu.h"

#include <doodle_core/metadata/kitsu/task_type.h>
#include <doodle_core/platform/win/register_file_type.h>
#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/kitsu/task.h>
#include <doodle_lib/http_method/kitsu/user.h>
namespace doodle::http {

namespace {
boost::asio::awaitable<tcp_stream_type_ptr> create_kitsu_proxy() {
  using co_executor_type = boost::asio::as_tuple_t<boost::asio::use_awaitable_t<>>;

  using resolver_t       = co_executor_type::as_default_on_t<boost::asio::ip::tcp::resolver>;
  using resolver_ptr     = std::shared_ptr<resolver_t>;

  auto l_tcp             = std::make_shared<tcp_stream_type>(co_await boost::asio::this_coro::executor);
  auto l_resolver        = std::make_shared<resolver_t>(co_await boost::asio::this_coro::executor);

  auto kitsu_url         = g_ctx().get<kitsu_ctx_t>().url_;
  boost::urls::url l_url{kitsu_url};
  boost::system::error_code l_ec{};
  boost::asio::ip::tcp::resolver::results_type l_result;

  std::tie(l_ec, l_result) =
      co_await l_resolver->async_resolve(l_url.host(), l_url.has_port() ? l_url.port() : l_url.scheme());
  if (l_ec) {
    default_logger_raw()->log(log_loc(), level::err, "async_resolve error: {}", l_ec.message());
    co_return nullptr;
  }
  l_tcp->expires_after(30s);
  std::tie(l_ec, std::ignore) = co_await l_tcp->async_connect(l_result);
  if (l_ec) {
    default_logger_raw()->log(log_loc(), level::err, "async_connect error: {}", l_ec.message());
    co_return nullptr;
  }
  l_tcp->expires_after(30s);

  co_return l_tcp;
}
}  // namespace

http_route_ptr create_kitsu_route() {
  auto l_router = std::make_shared<http_route>(create_kitsu_proxy);
  kitsu::user_reg(*l_router);
  kitsu::task_reg(*l_router);
  return l_router;
}

namespace kitsu {
namespace {
boost::asio::awaitable<void> init_context_impl() {
  auto l_c = co_await g_ctx().get<std::shared_ptr<doodle::kitsu::kitsu_client>>()->get_user_ctx();
  if (!l_c) default_logger_raw()->error(l_c.error());
  auto l_json = l_c.value();

  {
    std::map<std::string, project_helper::database_t> l_prj_maps{};
    {
      auto l_prjs = g_ctx().get<sqlite_database>().get_all<project_helper::database_t>();
      for (auto&& l : l_prjs) l_prj_maps.emplace(l.name_, l);
    }
    auto l_prj_install = std::make_shared<std::vector<project_helper::database_t>>();
    for (auto&& l_prj : l_json["projects"]) {
      auto l_name = l_prj["name"].get<std::string>();
      auto l_id   = boost::lexical_cast<uuid>(l_prj["id"].get<std::string>());
      if (!l_prj_maps.contains(l_name)) {
        l_prj_install
            ->emplace_back(project_helper::database_t{
                .uuid_id_          = core_set::get_set().get_uuid(),
                .name_             = l_name,
                .path_             = "C:/sy/doodle",
                .local_path_       = "C:/sy/doodle",
                .auto_upload_path_ = "C:/sy/doodle",
                .kitsu_uuid_       = l_id
            })
            .generate_names();

      } else if (l_prj_maps[l_name].kitsu_uuid_ != l_id) {
        l_prj_maps[l_name].kitsu_uuid_ = l_id;
        l_prj_install->emplace_back(l_prj_maps[l_name]);
      }
    }
    if (!l_prj_install->empty())
      if (auto l_r = co_await g_ctx().get<sqlite_database>().install_range(l_prj_install))
        default_logger_raw()->error("初始化检查 项目后插入数据库失败 {}", l_r.error());
  }

  {
    std::map<std::string, metadata::kitsu::task_type_t> l_task_maps{};
    {
      auto l_ts = g_ctx().get<sqlite_database>().get_all<metadata::kitsu::task_type_t>();
      for (auto&& l : l_ts) l_task_maps.emplace(l.name_, l);
    }
    auto l_install = std::make_shared<std::vector<metadata::kitsu::task_type_t>>();
    for (auto&& l_prj : l_json["task_types"]) {
      auto l_name = l_prj["name"].get<std::string>();
      auto l_id   = boost::lexical_cast<uuid>(l_prj["id"].get<std::string>());
      if (!l_task_maps.contains(l_name)) {
        auto& lv = l_install->emplace_back(metadata::kitsu::task_type_t{
            .uuid_id_ = core_set::get_set().get_uuid(), .name_ = l_name, .kitsu_uuid_ = l_id, .use_chick_files = false
        });
        if (l_name == "角色" || l_name == "地编模型" || l_name == "绑定") lv.use_chick_files = true;

      } else if (l_task_maps[l_name].kitsu_uuid_ != l_id) {
        l_task_maps[l_name].kitsu_uuid_ = l_id;
        l_install->emplace_back(l_task_maps[l_name]);
      }
    }
    if (!l_install->empty())
      if (auto l_r = co_await g_ctx().get<sqlite_database>().install_range(l_install))
        default_logger_raw()->error("初始化检查 task_type 后插入数据库失败 {}", l_r.error());
  }
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
    return boost::lexical_cast<uuid>(l_q.substr(l_it + 10, l_q.find('&', l_it) - l_it - 10));
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
  }
  return doodle::details::assets_type_enum::other;
}
}  // namespace kitsu

}  // namespace doodle::http