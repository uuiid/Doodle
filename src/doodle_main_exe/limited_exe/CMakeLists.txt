add_executable(
        doodle_limited_exe
        WIN32
        EXCLUDE_FROM_ALL
        main.cpp
        gui/app.cpp
        gui/app.h
        gui/window.h
        gui/window.cpp
        ../DoodleExe.manifest
        ../icon.ico
        ../win_exe.rc
)

set_target_properties(doodle_limited_exe
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(doodle_limited_exe
        PUBLIC
        doodle_lib
        )
target_compile_definitions(doodle_limited_exe
        PUBLIC
        #            WIN32_LEAN_AND_MEAN
        ISOLATION_AWARE_ENABLED
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        # ENTT_API_IMPORT
        )

# 定义安装逻辑

install(TARGETS doodle_limited_exe
        COMPONENT limited_exe_com
        #        RUNTIME_DEPENDENCIES
        )
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/
        DESTINATION bin/
        COMPONENT limited_exe_com
        )

# 最后实现camke修复
install(CODE "
include(BundleUtilities)
fixup_bundle(\"$<TARGET_FILE:doodle_limited_exe>\"   \"\"   \"\")
        "
        COMPONENT limited_exe_com
        )
