target_sources(
        doodle_lib
        PUBLIC
        dingding_attendance.h
        tool_version.h
        up_file.h
        http_jwt_fun.h
        seed_email.h
        kitsu.h

        PRIVATE
        dingding_attendance.cpp
        computer_reg_data.cpp
        tool_version.cpp
        up_file.cpp
        http_jwt_fun.cpp
        seed_email.cpp
        kitsu.cpp
)

add_subdirectory(kitsu)
add_subdirectory(model_library)
add_subdirectory(local)
add_subdirectory(other)
add_subdirectory(seedance2)
# add_subdirectory1(ai)

set_source_files_properties(
        dingding_attendance.cpp
        computer_reg_data.cpp

        TARGET_DIRECTORY doodle_lib
        PROPERTIES JOB_POOL_COMPILE sql_query_pool
)