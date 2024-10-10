//
// Created by TD on 24-9-26.
//

#pragma once
#include <doodle_lib/core/http/http_route.h>
#include <doodle_lib/http_method/kitsu/http_route_proxy.h>

namespace doodle::http {
void reg_kitsu_front_end_http(kitsu::http_route_proxy& in_route, const FSys::path& in_root);

}
