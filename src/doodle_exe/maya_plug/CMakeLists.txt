set(doodle_maya_plug_cpp
        MayaPlug.cpp
        MotionMayaPlugInit.cpp
        MotionMayaPlugInit.h
        gui/maya_plug_app.cpp
        gui/maya_plug_app.h
        gui/maya_windwos.cpp
        gui/maya_windwos.h
        gui/action/reference_attr_setting.cpp
        gui/action/reference_attr_setting.h
        gui/widgets/maya_tool_widget.cpp
        gui/widgets/maya_tool_widget.h

        )

add_library(doodle_plug MODULE
        ${doodle_maya_plug_cpp}
        ${PROJECT_SOURCE_DIR}/stl.natvis
        )


#set_target_properties(doodle_plug PROPERTIES
#        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
#        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
#        )
set_target_properties(doodle_plug PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        )

target_link_libraries(doodle_plug PRIVATE doodle_lib maya::maya_2020_all )
target_include_directories(doodle_plug PUBLIC ${PROJECT_SOURCE_DIR}/src/doodle_exe)
#设置maya插件后缀名
set_target_properties(
        doodle_plug PROPERTIES SUFFIX ".mll"
)
if (NOT RUN_COPY)
    target_compile_options(doodle_plug PRIVATE /Zi)
    target_link_options(doodle_plug PRIVATE /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF) 
endif()