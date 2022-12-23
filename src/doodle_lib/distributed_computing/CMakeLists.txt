target_sources(
        doodle_lib
        PUBLIC
        server.h

        PRIVATE
        server.cpp
)

target_link_libraries(doodle_lib
        PUBLIC

        libzmq
)