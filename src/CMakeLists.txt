# set(RESOURCE_SOURCE
#         core_resource.qrc)
# qt5_add_resources(RESOURE_FILE ${RESOURCE_SOURCE})
include_directories(${CMAKE_CURRENT_LIST_DIR})

add_subdirectory(loggerlib)
add_subdirectory(pinyinlib)
add_subdirectory(corelib)
add_subdirectory(doodle_GUI)
add_subdirectory(doodle_server)
add_subdirectory(plug)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(pinyin PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(logger PUBLIC ${CMAKE_CURRENT_LIST_DIR})
# target_include_directories(fileSystem PUBLIC ${CMAKE_CURRENT_LIST_DIR})






