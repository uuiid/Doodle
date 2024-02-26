add_subdirectory(get)

target_sources(
        doodle_lib
        PUBLIC
        computer.h
        PRIVATE
        computer.cpp
)