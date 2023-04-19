target_sources(doodle_core
        PUBLIC
        select.h
        sqlite_client.h
        sql_com.h

        PRIVATE
        select.cpp
        sqlite_client.cpp
        )
add_subdirectory(details)
