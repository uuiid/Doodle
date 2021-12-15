

set(MAYA_VERSION_LIST 2018 2020 1)


set(doodle_maya_plug_cpp
        maya_plug.cpp
        maya_plug_fwd.h
        gui/maya_plug_app.cpp
        gui/maya_plug_app.h
        gui/maya_windwos.cpp
        gui/maya_windwos.h
        gui/action/reference_attr_setting.cpp
        gui/action/reference_attr_setting.h
        gui/action/comm_check_scenes.h
        gui/action/comm_check_scenes.cpp
        gui/action/comm_play_blast.h
        gui/action/comm_play_blast.cpp
        gui/widgets/maya_tool_widget.cpp
        gui/widgets/maya_tool_widget.h
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
        )
add_custom_target(${PROJECT_NAME}_maya_plug)
foreach (MAYA_VERSION IN ITEMS 2018 2019 2020)
    message("build maya version ${MAYA_VERSION}")
    find_package(Maya ${MAYA_VERSION} REQUIRED)

    add_library(doodle_plug_${MAYA_VERSION} MODULE
            ${doodle_maya_plug_cpp}
            ${PROJECT_SOURCE_DIR}/stl.natvis
            )
    set_target_properties(doodle_plug_${MAYA_VERSION} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
            )
    target_link_libraries(doodle_plug_${MAYA_VERSION} PRIVATE doodle_lib maya::maya_${MAYA_VERSION}_all)
    target_include_directories(doodle_plug_${MAYA_VERSION} PUBLIC ${PROJECT_SOURCE_DIR}/src/doodle_exe)
    #设置maya插件后缀名
    set_target_properties(
            doodle_plug_${MAYA_VERSION} PROPERTIES SUFFIX ".mll"
    )
    if (NOT RUN_COPY)
        target_compile_options(doodle_plug_${MAYA_VERSION} PRIVATE /Zi)
        target_link_options(doodle_plug_${MAYA_VERSION} PRIVATE /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF)
    endif ()
    add_dependencies(${PROJECT_NAME}_maya_plug doodle_plug_${MAYA_VERSION})
endforeach ()


