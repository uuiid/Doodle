
set(doodle_maya_plug_cpp
        maya_plug.cpp
        maya_plug_fwd.h
        gui/maya_plug_app.cpp
        gui/maya_plug_app.h
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
        fmt/fmt_select_list.h
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
        maya_comm/upload_files_command.h
        maya_comm/upload_files_command.cpp

        )

if (DEFINED Maya_Version)
    find_package(Maya ${Maya_Version} MODULE)

    cmake_print_variables(MAYA_DEFAULT_LOCATION)
    #    set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)
    #    set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE Release)
    #    set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
    #    set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
    find_package(Qt5 COMPONENTS Core Widgets REQUIRED
            HINTS ${MAYA_DEFAULT_LOCATION})
    find_package(dem-bones CONFIG REQUIRED)


    if (TARGET maya::maya_all)
        set(maya_plug_name doodle_maya)
        configure_file(DoodleExe.manifest.in doodle_maya.manifest)
        configure_file(maya_plug.rc.in doodle_maya.rc)
        add_library(doodle_maya MODULE
                ${doodle_maya_plug_cpp}
                doodle_maya.manifest
                doodle_maya.rc
                ../doodle_exe/main/icon.ico
                )
        target_include_directories(doodle_maya PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
        set_target_properties(doodle_maya PROPERTIES
                RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
                LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
                )
        target_link_libraries(doodle_maya PRIVATE doodle_lib
                maya::maya_all
                Qt5::Core
                Qt5::Widgets
                DemBones::dem-bones
                Eigen3::Eigen
                )
        target_include_directories(doodle_maya PUBLIC ../doodle_exe)
        #设置maya插件后缀名
        set_target_properties(
                doodle_maya PROPERTIES SUFFIX "_${Maya_Version}.mll"
        )


        configure_file(doodle.mod.in ${PROJECT_BINARY_DIR}/plug/maya/doodle.mod)
        add_custom_command(
                TARGET doodle_maya
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/script/maya ${PROJECT_BINARY_DIR}/plug/maya
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../doodle_core/resource/maya_fun_tool.py ${PROJECT_BINARY_DIR}/plug/maya/scripts/scripts/
        )
        # 定义安装逻辑
        install(TARGETS doodle_maya
                DESTINATION maya/plug-ins
                COMPONENT maya_plug_com
                )

        install(DIRECTORY ${PROJECT_SOURCE_DIR}/script/maya/
                DESTINATION maya
                COMPONENT maya_plug_com)
        install(FILES ${CMAKE_CURRENT_LIST_DIR}/../doodle_core/resource/maya_fun_tool.py
                DESTINATION maya/scripts/scripts/
                COMPONENT maya_plug_com)





    endif ()

endif ()

