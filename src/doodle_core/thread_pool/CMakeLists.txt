target_sources(
        doodle_core
        PUBLIC
        thread_pool.h
        process_pool.h
        process_message.h
        process_task.h

        PRIVATE
        process_message.cpp
        process_pool.cpp
)
