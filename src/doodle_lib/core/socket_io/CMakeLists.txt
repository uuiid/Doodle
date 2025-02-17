target_sources(doodle_lib
        PUBLIC
        websocket_impl.h
        socket_io_core.h
        sid_data.h
        socket_io_ctx.h
        core_enum.h
        engine_io.h
        socket_io_packet.h

        PRIVATE
        websocket_impl.cpp
        socket_io_core.cpp
        sid_data.cpp
        socket_io_ctx.cpp
        engine_io.cpp
        socket_io_packet.cpp
)
