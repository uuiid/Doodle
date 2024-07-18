//
// Created by TD on 2023/12/29.
//

#include "file_association_http.h"

#include <doodle_core/core/app_service.h>
#include <doodle_core/database_task/sqlite_client.h>
#include <doodle_core/platform/win/register_file_type.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/core/scan_win_service.h>
#include <doodle_lib/http_method/file_association.h>

#include <wil/resource.h>
#include <wil/result.h>
#include <windows.h>

namespace doodle::launch {
struct file_association_http_args_t {
  std::string kitsu_ip_;
  std::string kitsu_port_;
  std::uint16_t port_;

  std::string kitsu_token_{};

  // form json
  friend void from_json(const nlohmann::json& in_json, file_association_http_args_t& out_obj) {
    if (in_json.contains("kitsu_ip")) in_json.at("kitsu_ip").get_to(out_obj.kitsu_ip_);
    if (in_json.contains("kitsu_port")) in_json.at("kitsu_port").get_to(out_obj.kitsu_port_);
    if (in_json.contains("kitsu_token")) in_json.at("kitsu_token").get_to(out_obj.kitsu_token_);
    in_json.at("port").get_to(out_obj.port_);
  }
};

bool file_association_http_t::operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
  app_base::Get().use_multithread(true);
  auto l_scan = g_ctx().emplace<std::shared_ptr<scan_win_service_t>>(std::make_shared<scan_win_service_t>());
  l_scan->start();

  file_association_http_args_t l_args{.kitsu_ip_ = "192.168.40.182", .kitsu_port_ = "80", .port_ = 50026};

  if (auto l_file_path = in_arh({"config"}); l_file_path) {
    auto l_json = nlohmann::json::parse(FSys::ifstream{FSys::from_quotation_marks(l_file_path.str())});
    try {
      l_args = l_json.get<file_association_http_args_t>();
    } catch (...) {
      default_logger_raw()->log(log_loc(), level::err, boost::current_exception_diagnostic_information());
      return true;
    }
  }
  default_logger_raw()->log(log_loc(), level::warn, "开始服务器");

  auto l_rout_ptr = std::make_shared<http::http_route>();
  default_logger_raw()->log(log_loc(), level::warn, "开始路由");
  http::reg_file_association_http(*l_rout_ptr);
  http::run_http_listener(g_io_context(), l_rout_ptr, l_args.port_);
  default_logger_raw()->log(log_loc(), level::warn, "启动侦听器");
  return false;
}
} // namespace doodle::launch