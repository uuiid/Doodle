add_subdirectory(get)

target_sources(
        doodle_lib
        PUBLIC
        computer.h
        task_info.h

        PRIVATE
        computer.cpp
        task_info.cpp
)