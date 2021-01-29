add_subdirectory(fileSystem)


target_sources(doodleServer
PUBLIC
server.h
Project.h
seting.h
FileSystem.h)

target_sources(doodleServer
PRIVATE
server.cpp
Project.cpp
seting.cpp
FileSystem.cpp)

target_include_directories(doodleServer
PUBLIC
${CMAKE_CURRENT_LIST_DIR})