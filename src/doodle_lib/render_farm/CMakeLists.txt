target_sources(
        doodle_lib
        PUBLIC
        server.h
        working_machine.h
        working_machine_session.h

        PRIVATE
        server.cpp
        working_machine.cpp
        working_machine_session.cpp
)
add_subdirectory(detail)