target_sources(
        doodle_lib
        PUBLIC
        client.h
        proxy_server.h
        working_machine.h
        working_machine_session.h

        PRIVATE
        client.cpp
        proxy_server.cpp
        working_machine.cpp
        working_machine_session.cpp
)
add_subdirectory(exception)
add_subdirectory(detail)
