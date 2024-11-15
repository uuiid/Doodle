target_sources(
        doodle_lib
        PUBLIC
        http_listener.h
        http_session_data.h
        http_route.h
        http_function.h
        json_body.h
        websocket_route.h
        http_websocket_client.h
        zlib_deflate_file_body.h


        PRIVATE
        http_listener.cpp
        http_session_data.cpp
        http_route.cpp
        http_function.cpp
        json_body.cpp
        websocket_route.cpp
        http_websocket_client.cpp
        zlib_deflate_file_body.cpp
)