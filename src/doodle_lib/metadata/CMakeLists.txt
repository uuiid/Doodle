target_sources(
        doodle_lib
        PUBLIC
        task_status.h
        entity.h
        status_automation.h
        comment.h

        PRIVATE
        task_status.cpp
        entity.cpp
        status_automation.cpp
        comment.cpp
)