target_sources(
        doodle_lib
        PUBLIC
        http_listener.h
        socket_logger.h
        http_session_data.h
        http_websocket_data.h
        http_route.h
        http_function.h
        json_body.h


        PRIVATE
        http_listener.cpp
        socket_logger.cpp
        http_session_data.cpp
        http_websocket_data.cpp
        http_route.cpp
        http_function.cpp
        json_body.cpp
)