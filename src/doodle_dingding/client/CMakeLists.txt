target_sources(doodle_dingding
        PUBLIC
        client.h
        dingding_api.h

        PRIVATE
        client.cpp
        dingding_api.cpp
        )
add_subdirectory(detail)
