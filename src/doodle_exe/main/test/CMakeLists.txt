
set(doodle_test_list
        test_core.cpp
        test_external.cpp
        test_metadata.cpp
        test_server.cpp
        test_gui.cpp
        test_image.cpp
        test_win.cpp
        test_std.cpp
        test_lib.cpp
        )
include(Catch)

foreach (_file_ IN LISTS doodle_test_list)
    add_executable(
            doodle_${_file_}
            ${_file_}
            ../DoodleExe.manifest
            ../icon.ico
            ../win_exe.rc
            main.cpp
    )
    set_target_properties(doodle_${_file_}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    target_link_libraries(doodle_${_file_}
            PUBLIC
            doodle_lib
            Catch2::Catch2
            )
    target_compile_definitions(doodle_${_file_}
            PUBLIC
            $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
            )
    target_compile_options(doodle_${_file_} PRIVATE $<$<CXX_COMPILER_ID:MSVC>: /bigobj>)# $<$<CXX_COMPILER_ID:GNU>: -Wa,-mbig-obj>
    catch_discover_tests(doodle_${_file_})
endforeach ()
