find_package(Qt5 CONFIG COMPONENTS Core REQUIRED)

set(LOG_FILES 
Logger.h
Logger.cpp
LoggerTemplate.h
)

add_library(logger SHARED 
    Logger.h
    Logger.cpp
    LoggerTemplate.h)

target_link_libraries(logger PUBLIC Qt5::Core)
target_compile_definitions(logger PRIVATE LOGGER_LIBRARY)

target_include_directories(logger PUBLIC ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS logger DESTINATION lib)
install(FILES ${LOG_FILES} DESTINATION include)
