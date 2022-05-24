
set(doodle_test_list
        test_core
        test_external
        test_metadata
        test_server
        test_gui
        test_image
        test_win
        test_std
        test_lib
        test_rpc
        )
include(Catch)

foreach (_file_ IN LISTS doodle_test_list)
    add_executable(
            doodle_${_file_}
            EXCLUDE_FROM_ALL
            ${_file_}.cpp
            ../DoodleExe.manifest
            ../icon.ico
            ../win_exe.rc

    )
    set_target_properties(doodle_${_file_}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    target_link_libraries(doodle_${_file_}
            PUBLIC
            doodle_lib
            Catch2::Catch2WithMain
            )
    target_compile_definitions(doodle_${_file_}
            PUBLIC
            $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
            )
    target_compile_options(doodle_${_file_} PRIVATE $<$<CXX_COMPILER_ID:MSVC>: /bigobj>)# $<$<CXX_COMPILER_ID:GNU>: -Wa,-mbig-obj>
    catch_discover_tests(doodle_${_file_})
endforeach ()
