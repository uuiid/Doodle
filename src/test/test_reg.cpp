#include "test_reg.h"

#include <doodle_lib/core/app_base.h>
#include <doodle_lib/http_method/kitsu.h>

namespace doodle::http {

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(test_health, get) {
  nlohmann::json l_json     = nlohmann::json::object();
  l_json["ok"]              = true;
  l_json["port"]            = port_;
  l_json["root"]            = root_;
  l_json["db_path"]         = db_path_;
  l_json["socket_io"]       = core_set::get_set().sid_ctx_ != nullptr;
  co_return std::make_unique<http_body_json_t>(l_json);
}

DOODLE_HTTP_FUN_OVERRIDE_IMPLEMENT(test_stop, get) {
  app_base::Get().stop_app();
  co_return std::make_unique<http_body_json_t>((nlohmann::json{} = "server stopping"));
}

http_route_ptr create_test_route(std::string in_root, std::string in_db_path, std::uint16_t in_port) {
  auto l_route = std::make_shared<http_route>();
  reg_kitsu_socket_io(*l_route);
  reg_kitsu_urls(*l_route);
  reg_kitsu_local_urls(*l_route);
  (*l_route)
      .reg_c<test_health>(in_root, in_db_path, in_port)
      .reg_c<test_stop>(in_root, in_db_path, in_port);
  return l_route;
}

}  // namespace doodle::http
