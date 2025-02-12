target_sources(doodle_lib
        PUBLIC
        websocket_impl.h
        socket_io_core.h

        PRIVATE
        websocket_impl.cpp
        socket_io_core.cpp
)
