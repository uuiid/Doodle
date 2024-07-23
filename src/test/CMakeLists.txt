

add_executable(
        test_main
        EXCLUDE_FROM_ALL
)
target_sources(
        test_main
        PRIVATE
        test_main.cpp
)

target_sources(
        test_main
        PUBLIC
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src/DoodleExe.manifest
        ${PROJECT_SOURCE_DIR}/src/icon.ico
        ${PROJECT_SOURCE_DIR}/src/win_exe.rc
)
set_target_properties(test_main
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(test_main
        PUBLIC
        doodle_lib
        Boost::unit_test_framework
        cldapi.lib
        Boost::log
        Boost::log_setup
        Alembic::Alembic
        Fbx::Fbx-static
)
target_compile_definitions(test_main
        PUBLIC
        CRTDBG_MAP_ALLOC
        BOOST_TEST_DYN_LINK
        BOOST_ALL_DYN_LINK
)

target_compile_options(test_main PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
        $<$<CXX_COMPILER_ID:MSVC>:/MP>)

add_subdirectory(core)
add_subdirectory(lib)
add_subdirectory(math)
add_subdirectory(ai)
add_subdirectory(http_client)
add_subdirectory(fbx_abc)


# create the testing file and list of tests
create_test_sourcelist(main_test
        CommonCxxTests.cxx
        core/abc.cpp
        core/sod_log.cpp
        core/error_enum.cpp
        core/folder_is_save.cpp
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


 
