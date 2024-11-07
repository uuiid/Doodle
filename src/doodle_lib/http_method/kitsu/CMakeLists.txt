target_sources(
        doodle_lib
        PUBLIC
        task.h
        kitsu.h
        user.h
        kitsu_front_end.h
        http_route_proxy.h
        project.h

        PRIVATE
        task.cpp
        kitsu.cpp
        user.cpp
        kitsu_front_end.cpp
        http_route_proxy.cpp
        project.cpp
)