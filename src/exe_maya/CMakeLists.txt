set(DOODLE_MAYA doodle_maya_${Maya_Version})
add_executable(
        ${DOODLE_MAYA}
        main.cpp
)

add_subdirectory(facet)


set_target_properties(${DOODLE_MAYA}
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(${DOODLE_MAYA}
        PUBLIC
        doodle_lib
        doodle_app
        doodle_maya_lib

        maya::maya_all
        DemBones::dem-bones
        Eigen3::Eigen
        OpenMP::OpenMP_CXX
        )

target_compile_definitions(${DOODLE_MAYA}
        PUBLIC
        #            WIN32_LEAN_AND_MEAN
        #        ISOLATION_AWARE_ENABLED
        # ENTT_API_IMPORT
        )

target_compile_options(
        doodle_maya
        PRIVATE

        $<$<CXX_COMPILER_ID:MSVC>:/MP>
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)

#target_link_options(${DOODLE_MAYA} PUBLIC /MAP:test.txt)
target_link_options(${DOODLE_MAYA} PUBLIC /ENTRY:mainCRTStartup)

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT exe_com)
include(InstallRequiredSystemLibraries)


install(TARGETS ${DOODLE_MAYA}
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities" "openmaya" "qt" "foundation" "metadata"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" "Maya${Maya_Version}"
        COMPONENT exe_com
        )

#doodle_install_code_wix(bin exe_com)
#configure_file(install.wxs install.wxs COPYONLY)
#configure_file(install_script.cmake install_script.cmake @ONLY)
#
#install(CODE
#        "
#set(DOODLE_RUN_NAME $<TARGET_FILE_NAME:${DOODLE_MAYA}>)
#set(DOODLE_MAIN_EXE $<TARGET_FILE:${DOODLE_MAYA}>)
#
#include(${CMAKE_CURRENT_BINARY_DIR}/install_script.cmake)
#        "
#        COMPONENT exe_com)