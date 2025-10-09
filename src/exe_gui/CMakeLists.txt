add_executable(
        doodle_kitsu_supplement
        kitsu_supplement_main.cpp
        ../DoodleExe.manifest
        ../icon.ico
        ../win_exe.rc
)

set_target_properties(doodle_kitsu_supplement
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(doodle_kitsu_supplement
        PUBLIC
        doodle_lib
)

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)

install(TARGETS doodle_kitsu_supplement
        RUNTIME_DEPENDENCIES
        DIRECTORIES
        $<TARGET_FILE_DIR:Fbx::Fbx>
        $ENV{CUDA_PATH}/bin
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)

set(DOODLE_INSTALL_IGNORE_ITEM
        doodle_maya_exe_2018.exe
        doodle_maya_exe_2019.exe
        doodle_maya_exe_2020.exe
)

list(JOIN DOODLE_INSTALL_IGNORE_ITEM " " DOODLE_INSTALL_IGNORE_ITEM)
# 最后实现camke修复
install(CODE
        "
# 修复dll
include(BundleUtilities)
fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_NAME:doodle_kitsu_supplement>.exe\" \"\" \"\${CMAKE_INSTALL_PREFIX}/bin\"
        IGNORE_ITEM
        ${DOODLE_INSTALL_IGNORE_ITEM}
)

# 生成授权文件
execute_process(
        COMMAND $<TARGET_FILE:generate_token> \${CMAKE_INSTALL_PREFIX}/bin/
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/bin
)
        "
)
