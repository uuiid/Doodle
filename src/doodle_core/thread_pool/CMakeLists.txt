target_sources(
        doodle_core
        PUBLIC
        process_message.h
        process_task.h
        image_to_movie.h

        PRIVATE
        process_message.cpp
        image_to_movie.cpp
)
