
set(doodle_maya_plug_cpp
        maya_plug.cpp
        maya_plug_fwd.h
        gui/maya_plug_app.cpp
        gui/maya_plug_app.h
        gui/maya_menu_bar.h
        gui/maya_menu_bar.cpp
        gui/action/reference_attr_setting.cpp
        gui/action/reference_attr_setting.h
        gui/action/comm_check_scenes.h
        gui/action/comm_check_scenes.cpp
        gui/action/comm_play_blast.h
        gui/action/comm_play_blast.cpp
        maya_render/hud_render_override.h
        maya_render/hud_render_override.cpp
        maya_render/hud_render_node.h
        maya_render/hud_render_node.cpp
        data/play_blast.h
        data/play_blast.cpp
        data/create_hud_node.h
        data/create_hud_node.cpp
        data/reference_file.h
        data/reference_file.cpp
        data/qcloth_shape.cpp
        data/qcloth_shape.h
        data/maya_file_io.cpp
        data/maya_file_io.h
        logger/maya_logger_info.h
        data/maya_camera.h
        data/maya_camera.cpp
        maya_comm/cam_comm.cpp
        maya_comm/cam_comm.h
        maya_comm/open_doodle_main.cpp
        maya_comm/open_doodle_main.h
        maya_comm/reference_comm.cpp
        maya_comm/reference_comm.h
        maya_comm/play_blash_comm.h
        maya_comm/play_blash_comm.cpp
        data/sim_cloth_tools.cpp
        data/sim_cloth_tools.h
        maya_comm/file_comm.cpp
        maya_comm/file_comm.h
        data/maya_tool.h
        data/maya_tool.cpp
        gui/action/create_sim_cloth.h
        gui/action/create_sim_cloth.cpp
        exception/exception.h
        fmt/fmt_warp.h
        maya_comm/afterimage_comm.h
        maya_comm/afterimage_comm.cpp
        data/sim_overr_attr.h
        data/sim_overr_attr.cpp
        data/maya_clear_scenes.cpp
        data/maya_clear_scenes.h
        maya_comm/clear_scene_comm.cpp
        maya_comm/clear_scene_comm.h
        data/maya_poly_info.cpp
        data/maya_poly_info.h
        data/find_duplicate_poly.cpp
        data/find_duplicate_poly.h
        maya_comm/find_duplicate_poly_comm.h
        maya_comm/find_duplicate_poly_comm.cpp
        configure/static_value.h
        gui/maya_layout.cpp
        gui/maya_layout.h
        maya_comm/auto_chick_call_back.cpp
        maya_comm/auto_chick_call_back.h
        maya_comm/replace_rig_file_command.h
        maya_comm/replace_rig_file_command.cpp
        data/maya_call_guard.h
        data/maya_call_guard.cpp

        )

if (DEFINED Maya_Version)
    find_package(Maya ${Maya_Version} MODULE)

    cmake_print_variables(MAYA_DEFAULT_LOCATION)
    #    find_package(Qt5 COMPONENTS Core Widgets REQUIRED
    #            HINTS ${MAYA_DEFAULT_LOCATION})


    if (TARGET maya::maya_all)

        add_library(doodle_maya_${Maya_Version} MODULE
                ${doodle_maya_plug_cpp}
                ../main/GUI/main.cpp
                ../main/DoodleExe.manifest
                ../main/icon.ico
                ../main/win_exe.rc
                )
        set_target_properties(doodle_maya_${Maya_Version} PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
                )
        target_link_libraries(doodle_maya_${Maya_Version} PRIVATE doodle_lib maya::maya_all)
        target_include_directories(doodle_maya_${Maya_Version} PUBLIC ${PROJECT_SOURCE_DIR}/src/doodle_exe)
        #设置maya插件后缀名
        set_target_properties(
                doodle_maya_${Maya_Version} PROPERTIES SUFFIX ".mll"
        )


        configure_file(doodle.mod.in ${PROJECT_BINARY_DIR}/plug/maya/doodle.mod)
        add_custom_command(
                TARGET doodle_maya_${Maya_Version}
                PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/script/maya ${PROJECT_BINARY_DIR}/plug/maya
                COMMAND ${CMAKE_COMMAND} -E copy ${PROJECT_SOURCE_DIR}/src/doodle_core/resource/maya_fun_tool.py ${PROJECT_BINARY_DIR}/plug/maya/scripts/scripts/
        )
    endif ()

endif ()

