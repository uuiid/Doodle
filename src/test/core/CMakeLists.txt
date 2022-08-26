add_executable(
        test_pinying
        EXCLUDE_FROM_ALL
        pinyin.cpp
)
doodle_link_test(test_pinying)
add_test(test_pinying_exe test_pinying)
