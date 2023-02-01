add_executable(
        DoodleExe
        #        WIN32
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
        #            WIN32_LEAN_AND_MEAN
        #        ISOLATION_AWARE_ENABLED
        # ENTT_API_IMPORT
        )

#target_link_options(DoodleExe PUBLIC /MAP:test.txt)

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
install(FILES ${PROJECT_BINARY_DIR}/bin/token.doodle_token
        DESTINATION bin/
        COMPONENT exe_com
        )
#最后实现camke修复
install(CODE [=[
        include(BundleUtilities)
        fixup_bundle("$<TARGET_FILE:DoodleExe>"   ""   "")
]=]
        COMPONENT exe_com
        )

doodle_install_code_wix(bin exe_com)
configure_file(install.wxs install.wxs COPYONLY)


install(CODE
        "
set(DOODLE_RUN_NAME $<TARGET_FILE_NAME:DoodleExe>)
configure_file(${CMAKE_CURRENT_LIST_DIR}/install.wxs \${CMAKE_INSTALL_PREFIX}/wix/doodle_gui_exe.wxs)
        "
        COMPONENT exe_com)




