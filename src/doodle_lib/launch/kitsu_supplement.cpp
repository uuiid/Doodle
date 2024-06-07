#include "kitsu_supplement.h"

#include <doodle_app/app/app_command.h>

#include <doodle_lib/core/http/http_listener.h>
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_client/kitsu_client.h>
#include <doodle_lib/http_method/computing_time.h>
#include <doodle_lib/http_method/dingding_attendance.h>
#include <doodle_lib/http_method/sqlite/kitsu_backend_sqlite.h>
namespace doodle::launch {
bool kitsu_supplement_t::operator()(const argh::parser& in_arh, std::vector<std::shared_ptr<void>>& in_vector) {
  auto& l_save = g_ctx().emplace<http::kitsu_backend_sqlite>();

  {
    g_pool_db().set_path("D:/test_files/test_db/test2.db");
    auto l_db_conn = g_pool_db().get_connection();
    l_save.init(l_db_conn);
  }
  auto l_client = g_ctx().emplace<std::shared_ptr<kitsu::kitsu_client>>(
      std::make_shared<kitsu::kitsu_client>("192.168.40.182", "80")
  );
  l_client->set_access_token(std::string{g_token});

  auto l_rout_ptr = std::make_shared<http::http_route>();
  http::reg_computing_time(*l_rout_ptr);
  http::reg_dingding_attendance(*l_rout_ptr);

  // 开始运行服务器
  auto l_listener = std::make_shared<http::http_listener>(g_thread().executor(), l_rout_ptr, 50023);
  l_listener->run();
  l_save.run();

  return false;
}
}  // namespace doodle::launch