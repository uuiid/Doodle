target_sources(
        doodle_lib
        PUBLIC
        sqlite_database.h
        sqlite_select_data.h
        sqlite_upgrade.h

        PRIVATE
        sqlite_database.cpp
        sqlite_select_data.cpp
        sqlite_upgrade.cpp
)
add_subdirectory(detail)
add_subdirectory(tokenizer)

set_source_files_properties(
        sqlite_database.cpp
        sqlite_select_data.cpp
        sqlite_upgrade.cpp
        TARGET_DIRECTORY doodle_lib
        PROPERTIES JOB_POOL_COMPILE sql_query_pool
)