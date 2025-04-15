target_sources(
        doodle_core
        PUBLIC
        sqlite_database.h
        sqlite_select_data.h

        PRIVATE
        sqlite_database.cpp
        sqlite_select_data.cpp
)
add_subdirectory(detail)