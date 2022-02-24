include(doodle_exe)

add_executable(
        DoodleExe
        WIN32
        DoodleGui_fwd.h
        main/GUI/main.cpp
        main/doodle.manifest
        main/icon.ico
        main/win_exe.rc
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
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        # ENTT_API_IMPORT
        )
add_subdirectory(maya_plug)
add_subdirectory(main/test)

