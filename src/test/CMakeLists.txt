
set(doodle_test_list

        )
include(Catch)

foreach (_file_ IN LISTS doodle_test_list)
    add_executable(
            doodle_${_file_}
            EXCLUDE_FROM_ALL
            ${_file_}.cpp
            ../doodle_exe/main/DoodleExe.manifest
            ../doodle_exe/main/icon.ico
            ../doodle_exe/main/win_exe.rc

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
