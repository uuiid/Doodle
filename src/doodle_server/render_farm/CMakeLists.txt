target_sources(
        doodle_server_lib
        PUBLIC
        work.h
        proxy_server.h
        logger_com.h

        PRIVATE
        work.cpp
        proxy_server.cpp
        logger_com.cpp
)
add_subdirectory(exception)
add_subdirectory(detail)
