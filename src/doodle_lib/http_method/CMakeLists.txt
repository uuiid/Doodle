add_subdirectory(get)

target_sources(
        doodle_lib
        PUBLIC
        computer.h
        task_info.h
        task_server.h
        http_snapshot.h
        file_exists.h
        task_sqlite_server.h
        computing_time.h

        PRIVATE
        computer.cpp
        task_info.cpp
        task_server.cpp
        http_snapshot.cpp
        file_exists.cpp
        task_sqlite_server.cpp
        computing_time.cpp
)