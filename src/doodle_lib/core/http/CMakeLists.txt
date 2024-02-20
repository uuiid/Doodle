target_sources(
        doodle_lib
        PUBLIC
        http_listener.h
        socket_logger.h
        http_session_data.h
        http_websocket_data.h

        PRIVATE
        http_listener.cpp
        socket_logger.cpp
        http_session_data.cpp
        http_websocket_data.cpp
)