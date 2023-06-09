add_executable(
        DoodleExe
        WIN32
        main.cpp
        ../DoodleExe.manifest
        ../icon.ico
        ../win_exe.rc
)

set_target_properties(DoodleExe
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(DoodleExe
        PUBLIC
        doodle_lib
)
target_compile_definitions(DoodleExe
        PUBLIC

        # WIN32_LEAN_AND_MEAN
        # ISOLATION_AWARE_ENABLED
        # ENTT_API_IMPORT
)

# target_link_options(DoodleExe PUBLIC /MAP:test.txt)
target_link_options(DoodleExe PUBLIC /ENTRY:mainCRTStartup)
target_link_options(DoodleExe PUBLIC)

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT exe_com)
include(InstallRequiredSystemLibraries)

install(TARGETS DoodleExe
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        COMPONENT exe_com
)
# 最后实现camke修复
install(CODE
        "
# 修复dll
include(BundleUtilities)
fixup_bundle(\"$<TARGET_FILE:DoodleExe>\" \"\" \"\"
        IGNORE_ITEM
        doodle_maya_exe_2018.exe
        doodle_maya_exe_2019.exe
        doodle_maya_exe_2020.exe
)

# 生成授权文件
execute_process(
        COMMAND $<TARGET_FILE:generate_token> ${CMAKE_INSTALL_PREFIX}/bin/
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)
        "
        COMPONENT exe_com)
