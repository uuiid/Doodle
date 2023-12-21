target_sources(
        doodle_lib
        PUBLIC
        image_to_move.h
        image_load_task.h
        connect_video.h

        PRIVATE
        image_to_move.cpp
        image_load_task.cpp
        connect_video.cpp
)