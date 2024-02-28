add_subdirectory(get)

target_sources(
        doodle_lib
        PUBLIC
        computer.h
        task_info.h
        task_server.h

        PRIVATE
        computer.cpp
        task_info.cpp
        task_server.cpp
)