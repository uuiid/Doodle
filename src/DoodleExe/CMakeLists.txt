include(doodle_exe)

add_custom_target(
        create_list_doodle_gui_cmake
        COMMAND ${CMAKE_COMMAND} -DDOODLE_INCLUDE_CMAKE=${PROJECT_SOURCE_DIR} -P ListFile.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

include(DoodleGui.cmake)

add_executable(
        DoodleExe
        WIN32
        ${DOODLELIB_GUI_HEADER}
        ${DOODLELIB_GUI_SOURCE}
        main/GUI/main.cpp
        main/doodle.manifest
        main/icon.ico
        main/win_exe.rc
)

set_target_properties(DoodleExe
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(DoodleExe
        PUBLIC
        DoodleLib
        unofficial::nana::nana
        )
target_compile_definitions(DoodleExe
        PUBLIC
        #            _WIN32_WINNT=0x0A00
        #            WIN32_LEAN_AND_MEAN
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        )



add_executable(
        DoodleServerExe
        main/SERVER/main.cpp
        main/doodle.manifest
        main/icon.ico
        main/win_exe.rc
)
set_target_properties(DoodleServerExe
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(DoodleServerExe
        PUBLIC
        DoodleLib
        )
target_compile_definitions(DoodleServerExe
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        )


add_executable(
        doodle_test
        main/test/main.cpp
        main/test/test_core.cpp
        main/test/test_external.cpp
        main/test/test_metadata.cpp
        main/test/test_server.cpp
        main/doodle.manifest
        main/icon.ico
        main/win_exe.rc
)
set_target_properties(doodle_test
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(doodle_test
        PUBLIC
        DoodleLib
        )
target_compile_definitions(doodle_test
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        )
target_compile_options(doodle_test PRIVATE /bigobj)
include(Catch)
catch_discover_tests(doodle_test)
