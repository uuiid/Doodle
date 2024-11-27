target_sources(
        doodle_core
        PUBLIC
        sqlite_database.h

        PRIVATE
        sqlite_database.cpp
)
add_subdirectory(detail)