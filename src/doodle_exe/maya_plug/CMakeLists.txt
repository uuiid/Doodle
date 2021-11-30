

set(MAYA_VERSION_LIST 2018 2020 1)


set(doodle_maya_plug_cpp
        MayaPlug.cpp
        maya_plug_fwd.h
        MotionMayaPlugInit.cpp
        MotionMayaPlugInit.h
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
        command/play_blast.h
        command/play_blast.cpp
        command/create_hud_node.h
        command/create_hud_node.cpp
        command/reference_file.h
        command/reference_file.cpp
        )
add_custom_target(${PROJECT_NAME}_maya_plug)
foreach (MAYA_VERSION IN ITEMS 2018 2019 2020)
    message("build maya version ${MAYA_VERSION}")
    find_package(Maya ${MAYA_VERSION} REQUIRED)

    add_library(doodle_plug_${MAYA_VERSION} MODULE
            ${doodle_maya_plug_cpp}
            ${PROJECT_SOURCE_DIR}/stl.natvis
            )


    #set_target_properties(doodle_plug PROPERTIES
    #        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
    #        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
    #        )
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


