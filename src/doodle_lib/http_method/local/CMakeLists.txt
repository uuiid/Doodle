target_sources(doodle_lib
        PUBLIC
        local.h
        PRIVATE
        local_setting.cpp
        task_info.cpp
        movie.cpp
)
set_source_files_properties(
        task_info.cpp

        TARGET_DIRECTORY doodle_lib
        PROPERTIES JOB_POOL_COMPILE sql_query_pool
)