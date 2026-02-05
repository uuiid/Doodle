target_sources(
        doodle_core
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

set_source_files_properties(
        sqlite_database.cpp
        TARGET_DIRECTORY doodle_core
        PROPERTIES JOB_POOL_COMPILE sql_query_pool
)