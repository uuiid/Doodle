add_executable(
        test_lib
        EXCLUDE_FROM_ALL
        holidaycn.cpp

)
doodle_link_test(test_lib)
add_test(NAME test_lib_exe COMMAND test_lib --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

