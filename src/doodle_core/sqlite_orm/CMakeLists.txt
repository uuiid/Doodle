target_sources(
        doodle_core
        PUBLIC
        sqlite_snapshot.h
        sqlite_base.h
        observer.h
        sqlite_database.h

        PRIVATE
        sqlite_snapshot.cpp
        observer.cpp
        sqlite_database.cpp
)
add_subdirectory(detail)