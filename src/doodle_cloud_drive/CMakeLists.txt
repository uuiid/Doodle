
add_library(
        doodle_cloud_drive_lib

        cloud_drive.h
)
target_link_libraries(doodle_cloud_drive_lib
        PUBLIC
        doodle_app
        cldapi.lib
        Shlwapi.lib
        )


add_executable(
        doodle_cloud_drive
        main.cpp
)

add_subdirectory(facet)
add_subdirectory(cloud)

set_target_properties(doodle_cloud_drive
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(doodle_cloud_drive
        PUBLIC
        doodle_app
        doodle_cloud_drive_lib
        )
target_compile_definitions(doodle_cloud_drive
        PUBLIC
        # WIN32_LEAN_AND_MEAN
        # ISOLATION_AWARE_ENABLED
        # ENTT_API_IMPORT
        )

# target_link_options(doodle_cloud_drive PUBLIC /MAP:test.txt)
#target_link_options(doodle_cloud_drive PUBLIC /ENTRY:mainCRTStartup)
#target_link_options(doodle_cloud_drive PUBLIC)

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT exe_com)
include(InstallRequiredSystemLibraries)

install(TARGETS doodle_cloud_drive
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        COMPONENT exe_com
        )
## 最后实现camke修复
#install(CODE
#        "
## 修复dll
#include(BundleUtilities)
#fixup_bundle(\"$<TARGET_FILE:doodle_cloud_drive>\" \"\" \"\"
#        IGNORE_ITEM
#        doodle_maya_exe_2018.exe
#        doodle_maya_exe_2019.exe
#        doodle_maya_exe_2020.exe
#)
#        "
#        COMPONENT exe_com)



