#pragma once

#include <doodle_core/doodle_core.h>
#include <doodle_core/doodle_core_fwd.h>
#include <doodle_core/lib_warp/cmrcWarp.h>
#include <doodle_core/lib_warp/sqlppWarp.h>
#include <doodle_core/lib_warp/std_warp.h>

#include <doodle_app/lib_warp/icon_font_macro.h>

#include <doodle_lib/configure/doodle_lib_export.h>
#include <doodle_lib/doodle_lib_pch.h>

// 开始我们的名称空间
namespace doodle {

namespace FSys {
DOODLELIB_API std::string file_hash_sha224(const path &in_file);
}

namespace distributed_computing {
class client;
class server;

using client_ptr = std::shared_ptr<client>;
using server_ptr = std::shared_ptr<server>;

}  // namespace distributed_computing
using namespace entt::literals;
using namespace std::literals;
using namespace chrono::literals;

class core_set;
class project;
class episodes;
class shot;
class assets;
class core_sql;
class assets_file;
class time_point_wrap;
class comment;
class season;
class work_task_info;

class core_sig;
class opencv_read_player;
class opencv_player_widget;
class image_icon;

class holidaycn_time;

}  // namespace doodle
