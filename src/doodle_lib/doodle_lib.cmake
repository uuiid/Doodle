set(
        DOODLELIB_HEADER
        app/app.h
        client/client.h
        core/ContainerDevice.h
        core/core_sql.h
        core/filesystem_extend.h
        core/observable_container.h
        gui/open_file_dialog.h
        core/program_options.h
        core/util.h
        core/image_loader.h
        ../doodle_core/core/status_info.h
        core/init_register.h
        core/authorization.h
        core/work_clock.h


        file_warp/maya_file.h
        file_warp/opencv_read_player.h
        gui/setting_windows.h
        gui/main_menu_bar.h
        gui/main_status_bar.h
        gui/get_input_dialog.h
        gui/gui_ref/path.h
        gui/gui_ref/project.h
        gui/gui_ref/ref_base.h
        gui/gui_ref/database_edit.h
        gui/gui_ref/base_windows_factory.h
        gui/gui_ref/base_window.h
        gui/gui_ref/layout_window.h

        gui/action/command_tool.h
        gui/widgets/assets_file_widgets.h
        gui/widgets/assets_filter_widget.h
        gui/widgets/edit_widgets.h
        gui/widgets/long_time_tasks_widget.h
        gui/widgets/opencv_player_widget.h
        gui/widgets/project_widget.h
        gui/widgets/screenshot_widget.h
        gui/widgets/project_edit.h
        gui/widgets/csv_export_widgets.h
        gui/widgets/time_sequencer_widget.h
        gui/widgets/ue4_widget.h
        gui/widgets/extract_subtitles_widgets.h
        gui/widgets/subtitle_processing.h
        gui/main_proc_handle.h

        lib_warp/WinReg.hpp
        lib_warp/boost_locale_warp.h
        lib_warp/cache.hpp
        lib_warp/cache_policy.hpp
        lib_warp/fifo_cache_policy.hpp
        lib_warp/imgui_warp.h
        lib_warp/lfu_cache_policy.hpp
        lib_warp/lru_cache_policy.hpp
        lib_warp/sqlppWarp.h


        toolkit/toolkit.h
        exe_warp/maya_exe.h
        exe_warp/ue4_exe.h
        long_task/image_to_move.h
        long_task/join_move.h
        long_task/restricted_task.h
        long_task/database_task.h
        long_task/drop_file_data.h
        long_task/short_cut.h
        long_task/image_load_task.h

        platform/win/drop_manager.h
        platform/win/list_drive.h
        platform/win/windows_alias.h
        platform/win/wnd_proc.h
        platform/win/register_file_type.h
        core/app_base.h
        doodle_lib_all.h
        doodle_lib_fwd.h
        doodle_lib_pch.h
)
set(
        DOODLELIB_SOURCE
        app/app.cpp
        client/client.cpp
        core/core_sql.cpp
        core/filesystem_extend.cpp
        core/program_options.cpp
        core/util.cpp
        core/image_loader.cpp
        ../doodle_core/core/status_info.cpp
        core/init_register.cpp
        core/authorization.cpp
        core/work_clock.cpp


        file_warp/maya_file.cpp
        file_warp/opencv_read_player.cpp
        gui/setting_windows.cpp
        gui/main_menu_bar.cpp
        gui/main_status_bar.cpp
        gui/get_input_dialog.cpp
        gui/gui_ref/path.cpp
        gui/gui_ref/project.cpp
        gui/gui_ref/ref_base.cpp
        gui/gui_ref/database_edit.cpp
        gui/gui_ref/base_windows_factory.cpp
        gui/gui_ref/base_window.cpp
        gui/gui_ref/layout_window.cpp

        gui/action/command_tool.cpp
        gui/widgets/assets_file_widgets.cpp
        gui/widgets/assets_filter_widget.cpp
        gui/widgets/edit_widgets.cpp
        gui/widgets/long_time_tasks_widget.cpp
        gui/widgets/opencv_player_widget.cpp
        gui/widgets/project_widget.cpp
        gui/widgets/screenshot_widget.cpp
        gui/widgets/project_edit.cpp
        gui/widgets/csv_export_widgets.cpp
        gui/widgets/time_sequencer_widget.cpp
        gui/widgets/ue4_widget.cpp
        gui/widgets/extract_subtitles_widgets.cpp
        gui/widgets/subtitle_processing.cpp
        gui/main_proc_handle.cpp

        gui/open_file_dialog.cpp


        lib_warp/imgui_warp.cpp


        toolkit/toolkit.cpp
        exe_warp/maya_exe.cpp
        exe_warp/ue4_exe.cpp
        long_task/image_to_move.cpp
        long_task/join_move.cpp
        long_task/restricted_task.cpp
        long_task/database_task.cpp
        long_task/drop_file_data.cpp
        long_task/short_cut.cpp
        long_task/image_load_task.cpp


        platform/win/drop_manager.cpp
        platform/win/list_drive.cpp
        platform/win/wnd_proc.cpp
        platform/win/register_file_type.cpp


        core/app_base.cpp
        doodle_lib_all.cpp
)
