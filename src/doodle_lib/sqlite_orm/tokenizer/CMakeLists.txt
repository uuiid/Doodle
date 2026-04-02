target_sources(
        doodle_lib
        PUBLIC
        sqlite_jieba.h

        PRIVATE
        sqlite_jieba.cpp
)


set_source_files_properties(
        sqlite_jieba.cpp
        TARGET_DIRECTORY doodle_lib
        PROPERTIES JOB_POOL_COMPILE sql_query_pool
)