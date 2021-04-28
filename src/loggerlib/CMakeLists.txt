
add_library(logger SHARED)
set_target_properties(logger 
PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)



target_sources(logger PRIVATE
Logger.cpp
        PUBLIC
        Logger.h
        LoggerTemplate.h
)
target_include_directories(logger PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(logger PUBLIC Boost::log )
#私有连接
target_link_libraries(logger PRIVATE Boost::locale Boost::filesystem)

target_compile_definitions(logger PRIVATE LOGGER_LIBRARY NOMINMAX)