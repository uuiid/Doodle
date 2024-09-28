//
// Created by TD on 24-8-21.
//

#include "user.h"

#include <doodle_core/sqlite_orm/sqlite_database.h>

#include <doodle_lib/core/http/json_body.h>
#include <doodle_lib/http_method/kitsu/kitsu.h>
namespace doodle::http::kitsu {
namespace {
boost::asio::awaitable<boost::beast::http::message_generator> user_context(session_data_ptr in_handle) {
  detail::http_client_data_base_ptr l_client_data = create_kitsu_proxy(in_handle);

  boost::beast::http::request<boost::beast::http::string_body> l_request{in_handle->req_header_};
  auto [l_ec, l_res]  = co_await detail::read_and_write<boost::beast::http::string_body>(l_client_data, l_request);
  try {
    if (l_res.result() == boost::beast::http::status::ok) {
      auto l_json = nlohmann::json::parse(l_res.body());

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
          in_handle->logger_->error("初始化检查 项目后插入数据库失败 {}", l_r.error());
    }
  } catch (...) {
    in_handle->logger_->warn("user_context error: {}", boost::current_exception_diagnostic_information());
  }
  co_return std::move(l_res);
}
}  // namespace
void user_reg(http_route& in_http_route) {
  in_http_route.reg(
      std::make_shared<http_function>(boost::beast::http::verb::get, "api/data/user/context", user_context)
  );
}
}  // namespace doodle::http::kitsu