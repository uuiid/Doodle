add_executable(
        test_pinying
        EXCLUDE_FROM_ALL
        pinyin.cpp
)
doodle_link_test(test_pinying)
add_test(NAME test_pinying_exe COMMAND test_pinying --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(
        test_lib_create
        EXCLUDE_FROM_ALL
        lib_create.cpp
)
doodle_link_test(test_lib_create)
add_test(NAME test_lib_create_exe COMMAND test_lib_create --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_executable(
        test_time
        EXCLUDE_FROM_ALL
        time.cpp
)
doodle_link_test(test_time)
add_test(NAME test_time_exe COMMAND test_time --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


add_executable(
        bvh_test
        EXCLUDE_FROM_ALL
        bvh.cpp
)
doodle_link_test(bvh_test)
add_test(NAME bvh_test_exe COMMAND bvh_test --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
