target_sources(
        doodle_lib
        PUBLIC
        work.h
        kitsu_client.h
        dingding_client.h

        PRIVATE
        work.cpp
        kitsu_client.cpp
        dingding_client.cpp
)