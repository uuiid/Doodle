add_executable(
        dingding_base
        EXCLUDE_FROM_ALL
        base.cpp
)
doodle_link_test(dingding_base)
add_test(dingding_base_ dingding_base)
