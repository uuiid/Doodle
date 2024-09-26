target_sources(
        doodle_lib
        PUBLIC
        task.h
        kitsu.h
        user.h
        kitsu_front_end.h

        PRIVATE
        task.cpp
        kitsu.cpp
        user.cpp
        kitsu_front_end.cpp
)