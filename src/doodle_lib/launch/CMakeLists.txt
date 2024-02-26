target_sources(
        doodle_lib
        PUBLIC
        main_gui.h
        auto_light_service.h
        http_distributed_tasks.h

        PRIVATE
        main_gui.cpp
        auto_light_service.cpp
        http_distributed_tasks.cpp

)