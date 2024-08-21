target_sources(
        doodle_lib
        PUBLIC
        task.h
        kitsu.h
        user.h

        PRIVATE
        task.cpp
        kitsu.cpp
        user.cpp
)