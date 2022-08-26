


macro(doodle_link_test)
    target_sources(
            ${ARGV0}
            PRIVATE
            ${PROJECT_SOURCE_DIR}/src/doodle_exe/main/DoodleExe.manifest
            ${PROJECT_SOURCE_DIR}/src/doodle_exe/main/icon.ico
            ${PROJECT_SOURCE_DIR}/src/doodle_exe/main/win_exe.rc
    )
#    set_target_properties(${ARGV0}
#            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    target_link_libraries(${ARGV0}
            PUBLIC
            doodle_lib
            Boost::unit_test_framework
            )
    target_compile_definitions(${ARGV0}
            PUBLIC
            $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
            )
    target_compile_options(${ARGV0} PRIVATE $<$<CXX_COMPILER_ID:MSVC>: /bigobj>)
    # $<$<CXX_COMPILER_ID:GNU>: -Wa,-mbig-obj>
endmacro()


add_subdirectory(core)
