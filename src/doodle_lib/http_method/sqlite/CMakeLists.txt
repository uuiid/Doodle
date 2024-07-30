target_sources(
        doodle_lib
        
        PUBLIC
        kitsu_backend_sqlite.h
        task_sqlite_server.h

        PRIVATE
        kitsu_backend_sqlite.cpp
        task_sqlite_server.cpp
)