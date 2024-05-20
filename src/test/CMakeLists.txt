

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
    )
    target_compile_definitions(${ARGV0}
            PUBLIC
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
add_subdirectory(math)
add_subdirectory(ai)
add_subdirectory(kitsu)
#add_subdirectory(rtt)
# 这一项要放在最后

add_subdirectory(fbx_abc)

add_subdirectory(main_fixtures)

# create the testing file and list of tests
create_test_sourcelist(main_test
        CommonCxxTests.cxx
        core/abc.cpp
        core/run_ue.cpp
        core/sod_log.cpp
        core/error_enum.cpp
        core/folder_is_save.cpp
        core/auto_light.cpp
        core/process_message.cpp
        core/merge_assets_tree.cpp
)

# add the executable
add_executable(CommonCxxTests ${main_test} ../DoodleExe.manifest)

# remove the test driver source file
set(TestsToRun ${main_test})
list(REMOVE_ITEM TestsToRun CommonCxxTests.cxx)

# Add all the ADD_TEST for each test
foreach (test ${TestsToRun})
    cmake_path(GET test PARENT_PATH TPath)
    cmake_path(GET test STEM TStem)
    if (TPath STREQUAL "")
        set(test_name ${TStem})
        set(test_comm ${TStem})
    else ()
        set(test_name ${TPath}_${TStem})
        set(test_comm ${TPath}/${TStem})
    endif ()
    add_test(NAME ${test_name} COMMAND CommonCxxTests ${test_comm})
endforeach ()

target_link_libraries(CommonCxxTests
        PUBLIC
        doodle_lib
        Alembic::Alembic
)
target_compile_definitions(CommonCxxTests
        PUBLIC
        BOOST_TEST_DYN_LINK
        BOOST_ALL_DYN_LINK
)
target_compile_options(CommonCxxTests PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: /bigobj>
        $<$<CXX_COMPILER_ID:MSVC>:/MP>)


add_executable(
        http_server
        http_server.cpp
)
doodle_link_test(http_server)
