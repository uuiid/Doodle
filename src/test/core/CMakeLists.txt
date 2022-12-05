add_executable(
        test_core
        EXCLUDE_FROM_ALL
        pinyin.cpp
        lib_create.cpp
        time.cpp
        bvh.cpp
        create_move.cpp
        boost_log.cpp
        rttr.cpp

)
target_link_libraries(
        test_core
        PUBLIC
        Boost::log
        Boost::log_setup
)
# target_compile_definitions(
#         test_core
#         PUBLIC
#         BOOST_LOG_DYN_LINK
#         BOOST_USE_WINAPI_VERSION=0x0A00
# )

doodle_link_test(test_core)
add_test(NAME test_core_exe COMMAND test_core --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
