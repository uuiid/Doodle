# set(RESOURCE_SOURCE
#         core_resource.qrc)
# qt5_add_resources(RESOURE_FILE ${RESOURCE_SOURCE})
include_directories(${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(corelib)
add_subdirectory(doodle_GUI)
add_subdirectory(doodle_server)
add_subdirectory(fileSystem)
add_subdirectory(loggerlib)
add_subdirectory(pinyinlib)








