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

install(TARGETS logger DESTINATION bin)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:logger>
        DESTINATION bin
        CONFIGURATIONS Debug)

#install(FILES ${LOG_FILES} DESTINATION include)
#
#if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
#    add_custom_command(
#            TARGET logger POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:logger> ${CMAKE_SOURCE_DIR}/install/bin/logger.dll
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:logger> ${CMAKE_SOURCE_DIR}/install/bin/logger.pdb
#    )
#else()
#    add_custom_command(
#            TARGET logger POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:logger> ${CMAKE_SOURCE_DIR}/install/bin/logger.dll
#    )
#endif()