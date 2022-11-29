add_executable(
        test_math
        EXCLUDE_FROM_ALL
        pca.cpp

)
target_link_libraries(
        test_math
        PUBLIC
        Boost::log
        Boost::log_setup
)
# target_compile_definitions(
#         test_math
#         PUBLIC
#         BOOST_LOG_DYN_LINK
#         BOOST_USE_WINAPI_VERSION=0x0A00
# )

doodle_link_test(test_math)
add_test(NAME test_math_exe COMMAND test_math --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)