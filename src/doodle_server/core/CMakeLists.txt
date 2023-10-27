target_sources(
        doodle_server_lib
        PUBLIC
        bind_front_handler.h
        http_listener.h
        http_session.h

        PRIVATE
        http_listener.cpp
        http_session.cpp
)