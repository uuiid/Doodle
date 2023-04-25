
set(maya_plug_name doodle_maya_lib)
configure_file(DoodleExe.manifest.in doodle_maya_lib.manifest)

#configure_file(maya_plug.rc.in doodle_maya_lib.rc)

add_library(doodle_maya_lib
        doodle_maya_lib.manifest
        #        doodle_maya_lib.rc
        ${PROJECT_SOURCE_DIR}/src/icon.ico
        )

add_subdirectory(configure configure_${Maya_Version})
add_subdirectory(data data_${Maya_Version})
add_subdirectory(exception exception_${Maya_Version})
add_subdirectory(fmt fmt_${Maya_Version})
add_subdirectory(gui gui_${Maya_Version})
add_subdirectory(logger logger_${Maya_Version})
add_subdirectory(main main_${Maya_Version})
add_subdirectory(maya_comm maya_comm_${Maya_Version})
add_subdirectory(maya_render maya_render_${Maya_Version})

target_include_directories(doodle_maya_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(doodle_maya_lib PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
        )
target_link_libraries(doodle_maya_lib
        PRIVATE
        doodle_core
        doodle_app

        maya::maya_all
        DemBones::dem-bones
        Eigen3::Eigen
        OpenMP::OpenMP_CXX
        )
target_compile_options(
        doodle_maya_lib
        PRIVATE

        $<$<CXX_COMPILER_ID:MSVC>:/MP>
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
