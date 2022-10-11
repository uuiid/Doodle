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
        #            _WIN32_WINNT=0x0A00
        #            WIN32_LEAN_AND_MEAN
        ISOLATION_AWARE_ENABLED
        # ENTT_API_IMPORT
        )

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT exe_com)
include(InstallRequiredSystemLibraries)

#list(APPEND DIRS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin)
#list(APPEND DIRS ${CMAKE_BINARY_DIR}/bin)

install(TARGETS DoodleExe
        #        RUNTIME_DEPENDENCIES
        COMPONENT exe_com
        )
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/
        DESTINATION bin/
        COMPONENT exe_com
        )

#最后实现camke修复
install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"$<TARGET_FILE:DoodleExe>\"   \"\"   \"${DIRS}\")
        "
        COMPONENT exe_com
        )


