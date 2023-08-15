target_sources(
        doodle_lib
        PUBLIC
        render_ue4.h
        url_route_base.h
        url_route_get.h
        url_route_post.h
        basic_json_body.h
        computer.h
        ue4_task.h
        aync_read_body.h
        forward_to_server.h

        PRIVATE
        render_ue4.cpp
        url_route_base.cpp
        url_route_get.cpp
        url_route_post.cpp
        basic_json_body.cpp
        computer.cpp
        ue4_task.cpp
        forward_to_server.cpp

)