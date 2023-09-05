

macro(doodle_link_test)
    target_sources(
            ${ARGV0}
            PUBLIC

            PRIVATE
            ${PROJECT_SOURCE_DIR}/src/DoodleExe.manifest
            ${PROJECT_SOURCE_DIR}/src/icon.ico
            ${PROJECT_SOURCE_DIR}/src/win_exe.rc
    )
    set_target_properties(${ARGV0}
            PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
    target_link_libraries(${ARGV0}
            PUBLIC
            doodle_lib
            Boost::unit_test_framework
            cldapi.lib
            doodle_cloud_drive_lib
    )
    target_compile_definitions(${ARGV0}
            PUBLIC
            $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
            CRTDBG_MAP_ALLOC
            BOOST_TEST_DYN_LINK
            BOOST_ALL_DYN_LINK
    )
    target_include_directories(
            ${ARGV0}
            PRIVATE
            ${PROJECT_SOURCE_DIR}/src/test
    )
    target_compile_options(${ARGV0} PRIVATE
            $<$<CXX_COMPILER_ID:MSVC>: /bigobj>
            $<$<CXX_COMPILER_ID:MSVC>:/MP>)
endmacro()

add_executable(
        test_main
        EXCLUDE_FROM_ALL
)

add_subdirectory(core)
add_subdirectory(lib)
add_subdirectory(rpc_test)
add_subdirectory(math)
add_subdirectory(ai)
# 这一项要放在最后

add_subdirectory(main_fixtures)

# create the testing file and list of tests
create_test_sourcelist(main_test
        CommonCxxTests.cxx
        http_proxy.cpp
        http_server.cpp
        udp_server.cpp
)

# add the executable
add_executable(CommonCxxTests ${main_test})

# remove the test driver source file
set(TestsToRun ${main_test})
remove(TestsToRun CommonCxxTests.cxx)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    get_filename_component(TName ${test} NAME_WE)
    add_test(NAME ${TName} COMMAND CommonCxxTests ${TName})
endforeach ()

target_link_libraries(CommonCxxTests
        PUBLIC
        doodle_lib
        Boost::unit_test_framework
)
target_compile_definitions(CommonCxxTests
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        BOOST_TEST_DYN_LINK
        BOOST_ALL_DYN_LINK
)
target_compile_options(CommonCxxTests PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: /bigobj>
        $<$<CXX_COMPILER_ID:MSVC>:/MP>)
