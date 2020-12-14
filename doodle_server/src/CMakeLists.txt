target_sources(doodleServer
PUBLIC
server.h)

target_sources(doodleServer
PRIVATE
server.cpp)

target_include_directories(doodleServer
PUBLIC
${CMAKE_CURRENT_LIST_DIR})