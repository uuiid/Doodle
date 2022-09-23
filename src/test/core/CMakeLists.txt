add_executable(
        test_pinying
        EXCLUDE_FROM_ALL
        pinyin.cpp
)
doodle_link_test(test_pinying)
add_test(test_pinying_exe test_pinying)

add_executable(
        test_lib_create
        EXCLUDE_FROM_ALL
        lib_create.cpp
)
doodle_link_test(test_lib_create)
add_test(test_lib_create_exe test_lib_create)


add_executable(
        test_time
        EXCLUDE_FROM_ALL
        time.cpp
)
doodle_link_test(test_time)
add_test(test_time_exe test_time)


add_executable(
        bvh_test
        EXCLUDE_FROM_ALL
        bvh.cpp
)
doodle_link_test(bvh_test)
add_test(bvh_test_exe bvh_test)
