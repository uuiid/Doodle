
add_library(logger SHARED)


target_sources(logger PUBLIC 
Logger.h
LoggerTemplate.h
)
target_sources(logger PRIVATE
Logger.cpp
)

target_link_libraries(logger PUBLIC Boost::log )
#私有连接
target_link_libraries(logger PRIVATE Boost::locale Boost::filesystem)

target_compile_definitions(logger PRIVATE LOGGER_LIBRARY)



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