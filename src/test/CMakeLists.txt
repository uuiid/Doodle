

macro(doodle_link_test)
        target_sources(
                ${ARGV0}
                PUBLIC

                PRIVATE
                ${PROJECT_SOURCE_DIR}/src/doodle_exe/main/DoodleExe.manifest
                ${PROJECT_SOURCE_DIR}/src/doodle_exe/main/icon.ico
                ${PROJECT_SOURCE_DIR}/src/doodle_exe/main/win_exe.rc
        )
        set_target_properties(${ARGV0}
                PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
        target_link_libraries(${ARGV0}
                PUBLIC
                doodle_lib
                doodle_dingding
                Boost::unit_test_framework
        )
        target_compile_definitions(${ARGV0}
                PUBLIC
                $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
                CRTDBG_MAP_ALLOC
        )
        target_include_directories(
                ${ARGV0}
                PRIVATE
                ${PROJECT_SOURCE_DIR}/src/test
        )
        target_compile_options(${ARGV0} PRIVATE $<$<CXX_COMPILER_ID:MSVC>: /bigobj>)
endmacro()

add_subdirectory(core)
add_subdirectory(dingding)
add_subdirectory(main_fixtures)
