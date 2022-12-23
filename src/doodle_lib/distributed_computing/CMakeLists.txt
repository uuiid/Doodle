find_package(cppzmq CONFIG REQUIRED)

target_sources(
        doodle_lib
        PUBLIC
        server.h
        client.h

        PRIVATE
        server.cpp
        client.cpp
)

target_link_libraries(doodle_lib
        PUBLIC

        cppzmq
)