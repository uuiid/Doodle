target_sources(
        doodle_core
        PUBLIC
        process_message.h
        image_to_movie.h
        process_callback.h

        PRIVATE
        process_message.cpp
        image_to_movie.cpp
        process_callback.cpp
)
