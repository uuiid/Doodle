target_sources(
        doodle_core
        PUBLIC
        thread_pool.h
        process_pool.h
        process_message.h
        process_task.h
        image_to_movie.h

        PRIVATE
        process_message.cpp
        process_pool.cpp
        image_to_movie.cpp
)
