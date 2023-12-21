target_sources(
        doodle_core
        PUBLIC
        process_message.h
        image_to_movie.h
        process_callback.h
        connect_video_interface.h

        PRIVATE
        process_message.cpp
        image_to_movie.cpp
        process_callback.cpp
        connect_video_interface.cpp
)
