
set(maya_plug_name doodle_maya_lib)
configure_file(DoodleExe.manifest.in doodle_maya_lib.manifest)

# configure_file(maya_plug.rc.in doodle_maya_lib.rc)
add_library(doodle_maya_lib
        doodle_maya_lib.manifest
        maya_plug_fwd.h
        # doodle_maya_lib.rc
        ${PROJECT_SOURCE_DIR}/src/icon.ico
)

add_subdirectory(configure configure_${Maya_Version})
add_subdirectory(abc abc_${Maya_Version})
add_subdirectory(data data_${Maya_Version})
add_subdirectory(exception exception_${Maya_Version})
add_subdirectory(fmt fmt_${Maya_Version})
add_subdirectory(logger logger_${Maya_Version})
add_subdirectory(main main_${Maya_Version})
add_subdirectory(maya_comm maya_comm_${Maya_Version})
add_subdirectory(node node_${Maya_Version})

target_include_directories(doodle_maya_lib PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(doodle_maya_lib PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
)
target_link_libraries(doodle_maya_lib
        PRIVATE
        doodle_core

        maya::maya_all
        DemBones::dem-bones
        Eigen3::Eigen
        OpenMP::OpenMP_CXX
        Alembic::Alembic
        Fbx::Fbx-static
)

target_compile_options(
        doodle_maya_lib
        PRIVATE

        $<$<CXX_COMPILER_ID:MSVC>:/MP>
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)


#target_compile_options(
#        doodle_maya_lib
#        PRIVATE
#        $<$<CXX_COMPILER_ID:MSVC>:/EHa>
#)
#
#string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#string(REPLACE "/EHs" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
#string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
#string(REPLACE "/EHs" "" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")