add_executable(
        test_pinying
        EXCLUDE_FROM_ALL
        pinyin.cpp
        lib_create.cpp
        time.cpp
        bvh.cpp

)
doodle_link_test(test_pinying)
add_test(NAME test_pinying_exe COMMAND test_pinying --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

