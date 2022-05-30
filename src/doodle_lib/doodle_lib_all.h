//
// Created by TD on 2021/5/9.
//

#pragma once
#include <doodle_lib/core/ContainerDevice.h>
#include <doodle_core/core/core_set.h>
#include <doodle_core/core/core_sql.h>
#include <doodle_core/core/doodle_lib.h>
#include <doodle_lib/core/filesystem_extend.h>
#include <doodle_lib/gui/open_file_dialog.h>
#include <doodle_lib/core/program_options.h>
#include <doodle_lib/core/util.h>

#include <doodle_lib/file_warp/opencv_read_player.h>
#include <doodle_lib/gui/setting_windows.h>
#include <doodle_lib/gui/action/command_tool.h>
#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/gui/widgets/edit_widgets.h>
#include <doodle_lib/gui/widgets/long_time_tasks_widget.h>
#include <doodle_lib/gui/widgets/opencv_player_widget.h>
#include <doodle_lib/gui/widgets/project_widget.h>
#include <doodle_lib/lib_warp/WinReg.hpp>
#include <doodle_lib/lib_warp/boost_locale_warp.h>
#include <doodle_core/lib_warp/boost_uuid_warp.h>
#include <doodle_lib/lib_warp/cache.hpp>
#include <doodle_lib/lib_warp/cache_policy.hpp>
#include <doodle_core/lib_warp/entt_warp.h>
#include <doodle_lib/lib_warp/fifo_cache_policy.hpp>
#include <doodle_lib/lib_warp/imgui_warp.h>
#include <doodle_core/lib_warp/json_warp.h>
#include <doodle_lib/lib_warp/lfu_cache_policy.hpp>
#include <doodle_lib/lib_warp/lru_cache_policy.hpp>
#include <doodle_lib/lib_warp/protobuf_warp.h>
#include <doodle_lib/lib_warp/protobuf_warp_cpp.h>
#include <doodle_lib/lib_warp/sqlppWarp.h>
#include <doodle_core/lib_warp/std_warp.h>
#include <doodle_lib/toolkit/toolkit.h>
#include <doodle_lib/exe_warp/maya_exe.h>
#include <doodle_lib/exe_warp/ue4_exe.h>
#include <doodle_lib/long_task/image_to_move.h>
#include <doodle_lib/long_task/join_move.h>
#include <doodle_core/thread_pool/process_pool.h>
#include <doodle_lib/platform/win/drop_manager.h>
#include <doodle_lib/platform/win/list_drive.h>
#include <doodle_lib/doodle_lib_fwd.h>
#include <doodle_lib/doodle_lib_pch.h>
#include <doodle_core/doodle_macro.h>

#include <doodle_lib/gui/widgets/assets_file_widgets.h>
#include <doodle_lib/gui/widgets/assets_filter_widget.h>
#include <doodle_lib/gui/widgets/csv_export_widgets.h>
#include <doodle_lib/gui/widgets/edit_widgets.h>
#include <doodle_lib/gui/widgets/long_time_tasks_widget.h>
#include <doodle_lib/gui/widgets/opencv_player_widget.h>
#include <doodle_lib/gui/widgets/project_edit.h>
#include <doodle_lib/gui/widgets/project_widget.h>
#include <doodle_lib/gui/widgets/screenshot_widget.h>
#include <doodle_lib/gui/widgets/time_sequencer_widget.h>
#include <doodle_lib/gui/widgets/ue4_widget.h>
#include <doodle_lib/gui/widgets/extract_subtitles_widgets.h>
#include <doodle_lib/gui/widgets/subtitle_processing.h>
#include <doodle_lib/gui/action/command_tool.h>
