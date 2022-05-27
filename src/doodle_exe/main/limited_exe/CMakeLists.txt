add_executable(
        doodle_limited_exe
        WIN32
        main.cpp
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
        #            _WIN32_WINNT=0x0A00
        #            WIN32_LEAN_AND_MEAN
        ISOLATION_AWARE_ENABLED
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        # ENTT_API_IMPORT
        )
add_dependencies(doodle_limited_exe generate_token_run)

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)

#list(APPEND DIRS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin)
#list(APPEND DIRS ${CMAKE_BINARY_DIR}/bin)

install(TARGETS doodle_limited_exe
        #        RUNTIME_DEPENDENCIES
        )
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/
        DESTINATION bin/)

#最后实现camke修复
install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"$<TARGET_FILE:doodle_limited_exe>\"   \"\"   \"${DIRS}\")
        "
        )
