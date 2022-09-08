set(DINGDING_KEY "${DINGDING_KEY}")
set(DINGDING_VALUE "${DINGDING_VALUE}")







configure_file(config.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/config.cpp)


target_sources(doodle_dingding
        PUBLIC
        doodle_dingding_export.h
        config.h

        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/config.cpp

        )


