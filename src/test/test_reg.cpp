#include "test_reg.h"

#include "test_debug_sql.h"
#include <doodle_lib/http_method/kitsu.h>

namespace doodle::http {

http_route_ptr create_test_route() {
  auto l_route = std::make_shared<http_route>();
  reg_kitsu_socket_io(*l_route);
  reg_kitsu_urls(*l_route);
  reg_kitsu_local_urls(*l_route);
  l_route->reg_c<debug_sql>();
  return l_route;
}

}  // namespace doodle::http
