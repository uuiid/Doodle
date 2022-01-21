include(doodle_exe)

add_custom_target(
        create_list_doodle_gui_cmake
        COMMAND ${CMAKE_COMMAND} -DDOODLE_INCLUDE_CMAKE=${PROJECT_SOURCE_DIR} -P ListFile.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

include(DoodleGui.cmake)

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
        doodle_lib
        )
target_compile_definitions(DoodleServerExe
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        # ENTT_API_IMPORT
        )

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
        doodle_lib
        )
target_compile_definitions(DoodleExe
        PUBLIC
        #            _WIN32_WINNT=0x0A00
        #            WIN32_LEAN_AND_MEAN
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        # ENTT_API_IMPORT
        )
#if (unofficial-nana_FOUND)
#
#else ()
#    #    add_executable(doodle_exe ALIAS DoodleServerExe )
#endif ()
add_subdirectory(maya_plug)
add_subdirectory(main/test)
add_subdirectory(main/test_gui)

