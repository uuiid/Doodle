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
add_subdirectory(orm)
