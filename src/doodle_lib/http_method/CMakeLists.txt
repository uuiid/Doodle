add_subdirectory(get)

target_sources(
        doodle_lib
        PUBLIC
        computer.h
        task_info.h
        task_server.h
        save_http_data.h
        http_snapshot.h

        PRIVATE
        computer.cpp
        task_info.cpp
        task_server.cpp
        save_http_data.cpp
        http_snapshot.cpp
)