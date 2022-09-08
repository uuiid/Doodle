set(DINGDING_KEY "0")
set(DINGDING_VALUE "0")







configure_file(config.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.cpp)


target_sources(doodle_dingding
        PUBLIC
        doodle_dingding_export.h
        config.h

        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/config.cpp

        )


