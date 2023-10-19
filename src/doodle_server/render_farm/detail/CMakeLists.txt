target_sources(
        doodle_server_lib

        PUBLIC
        url_route_base.h
        url_route_get.h
        url_route_post.h
        url_route_put.h
        computer.h
        ue4_task.h
        aync_read_body.h
        ue_task_manage.h
        computer_manage.h
        url_webscoket.h

        PRIVATE
        url_route_base.cpp
        url_route_get.cpp
        url_route_post.cpp
        url_route_put.cpp
        computer.cpp
        ue4_task.cpp
        ue_task_manage.cpp
        computer_manage.cpp
        url_webscoket.cpp
)