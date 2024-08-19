#pragma once

#include <doodle_core/doodle_core.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/cmrcWarp.h>
#include <doodle_core/lib_warp/sqlppWarp.h>
#include <doodle_core/lib_warp/std_warp.h>


#include <doodle_lib/configure/doodle_lib_export.h>
#include <doodle_lib/doodle_lib_pch.h>

// 开始我们的名称空间
namespace doodle {

namespace FSys {
DOODLELIB_API std::string file_hash_sha224(const path &in_file);
}

namespace render_farm {
namespace detail {
struct basic_json_body;
class http_route;
class ue4_task;
class render_ue4;

}  // namespace detail
using render_ue4     = detail::render_ue4;
using render_ue4_ptr = std::shared_ptr<render_ue4>;
using http_route_ptr = std::shared_ptr<detail::http_route>;

}  // namespace render_farm

namespace detail {
struct process_child;
}
using process_child_ptr = std::shared_ptr<detail::process_child>;
using namespace entt::literals;
using namespace std::literals;
using namespace chrono::literals;

class core_set;
class project;
class episodes;
class shot;
class assets;
class assets_file;
class time_point_wrap;
class comment;
class season;

class core_sig;
class opencv_read_player;
class opencv_player_widget;
class image_icon;

class holidaycn_time;
class udp_client;
using udp_client_ptr = std::shared_ptr<udp_client>;
}  // namespace doodle
