add_executable(
        test_core
        EXCLUDE_FROM_ALL
        pinyin.cpp
        lib_create.cpp
        time.cpp
        bvh.cpp

)
doodle_link_test(test_core)
add_test(NAME test_core_exe COMMAND test_core --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

