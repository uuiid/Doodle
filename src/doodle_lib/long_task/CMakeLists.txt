target_sources(
        doodle_lib
        PUBLIC
        image_to_move.h
        join_move.h
        image_load_task.h

        PRIVATE
        image_to_move.cpp
        join_move.cpp
        image_load_task.cpp
)