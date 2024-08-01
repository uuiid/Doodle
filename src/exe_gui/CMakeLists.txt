add_executable(
        DoodleExe
        WIN32
        main_gui.cpp
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
# target_link_options(DoodleExe PUBLIC /MAP:test.txt)
target_link_options(DoodleExe PUBLIC /ENTRY:mainCRTStartup)

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)


install(TARGETS DoodleExe
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)


add_executable(
        doodle
        WIN32
        main_gui2.cpp
        ../DoodleExe.manifest
        ../icon.ico
        ../win_exe.rc
)

set_target_properties(doodle
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(doodle
        PUBLIC
        doodle_lib
)
# target_link_options(doodle PUBLIC /MAP:test.txt)
target_link_options(doodle PUBLIC /ENTRY:mainCRTStartup)




install(TARGETS doodle
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
)







set(DOODLE_INSTALL_IGNORE_ITEM
        doodle_maya_exe_2018.exe
        doodle_maya_exe_2019.exe
        doodle_maya_exe_2020.exe
)

macro(add_doodle_exe _name _src)
    add_executable(
            ${_name}
            ${_src}
            ../DoodleExe.manifest
            ../icon.ico
            ../win_exe.rc
    )

    set_target_properties(${_name}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    target_link_libraries(${_name}
            PUBLIC
            doodle_lib
    )
    install(TARGETS ${_name}
            RUNTIME_DEPENDENCIES
            PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities"
            POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
    )
    list(APPEND DOODLE_INSTALL_IGNORE_ITEM ${_name}.exe)
endmacro()
add_doodle_exe(doodle_auto_light_server auto_light_server.cpp)
add_doodle_exe(doodle_auto_light_client auto_light_client.cpp)
add_doodle_exe(file_association_http file_association_http_main.cpp)
add_doodle_exe(doodle_auto_light_process auto_light_process.cpp)
add_doodle_exe(doodle_file_exists file_exists_launch.cpp)
add_doodle_exe(doodle_kitsu_supplement kitsu_supplement_main.cpp)

list(JOIN DOODLE_INSTALL_IGNORE_ITEM " " DOODLE_INSTALL_IGNORE_ITEM)
# 最后实现camke修复
install(CODE
        "
# 修复dll
include(BundleUtilities)
fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_NAME:DoodleExe>.exe\" \"\" \"\${CMAKE_INSTALL_PREFIX}/bin\"
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
