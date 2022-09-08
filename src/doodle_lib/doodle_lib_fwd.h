#pragma once

#include <doodle_lib/configure/doodle_lib_export.h>
#include <doodle_lib/doodle_lib_pch.h>
#include <doodle_core/doodle_core.h>
#include <doodle_core/lib_warp/cmrcWarp.h>

#include <doodle_core/lib_warp/sqlppWarp.h>
#include <doodle_core/lib_warp/std_warp.h>

#include <doodle_lib/lib_warp/icon_font_macro.h>

#include <doodle_core/doodle_core_fwd.h>

// 开始我们的名称空间
namespace doodle {

namespace FSys {
DOODLELIB_API std::string file_hash_sha224(const path &in_file);

}

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
class thread_pool;
class season;
class setting_windows;
class core_sig;
class app;
class program_options;
class short_cut;
class opencv_read_player;
class opencv_player_widget;
class image_icon;

class strand_gui;

strand_gui g_strand_gui();

using program_options_ptr = std::shared_ptr<program_options>;

}  // namespace doodle
