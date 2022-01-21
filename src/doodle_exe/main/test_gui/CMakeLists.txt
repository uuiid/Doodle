
set(doodle_test_list

        )

foreach (_file_ IN LISTS doodle_test_list)
    add_executable(
            doodle_${_file_}
            ${_file_}
            ../doodle.manifest
            ../icon.ico
            ../win_exe.rc
    )
    set_target_properties(doodle_${_file_}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    target_link_libraries(doodle_${_file_}
            PUBLIC
            doodle_lib
            )
    target_compile_definitions(doodle_${_file_}
            PUBLIC
            $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
            )
    target_compile_options(doodle_${_file_} PRIVATE $<$<CXX_COMPILER_ID:MSVC>: /bigobj>)# $<$<CXX_COMPILER_ID:GNU>: -Wa,-mbig-obj>
endforeach ()
