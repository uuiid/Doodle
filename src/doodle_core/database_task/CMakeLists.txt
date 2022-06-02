target_sources(doodle_core
        PUBLIC
        delete_data.h
        insert.h
        observe.h
        select.h
        update.h
        sqlite_client.h

        PRIVATE

        delete_data.cpp
        insert.cpp
        observe.cpp
        select.cpp
        update.cpp
        sqlite_client.cpp

        )
add_subdirectory(details)
