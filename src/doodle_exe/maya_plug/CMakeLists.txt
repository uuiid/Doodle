set(doodle_maya_plug_cpp
        MayaPlug.cpp
        MotionMayaPlugInit.cpp
        MotionMayaPlugInit.h
        )

add_library(doodle_plug MODULE
        ${doodle_maya_plug_cpp}
        )


set_target_properties(doodle_plug PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
        )
target_link_libraries(doodle_plug PRIVATE doodle_lib maya::maya_2020_all )
target_include_directories(doodle_plug PUBLIC ${CMAKE_CURRENT_LIST_DIR})
#设置maya插件后缀名
set_target_properties(
        doodle_plug PROPERTIES SUFFIX ".mll"
)

target_compile_definitions(
        doodle_plug
        PRIVATE
        -DPARTIO_WIN32
        -DNT_PLUGIN
        -D_WIN32
        -DOSWin_
        -DUSERDLL
        -DCRT_SECURE_NO_DEPRECATE
        -D_HAS_ITERATOR_DEBUGGING=0
        -D_CRT_SECURE_NO_WARNINGS
)
