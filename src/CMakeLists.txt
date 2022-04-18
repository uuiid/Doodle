# set(RESOURCE_SOURCE
#         core_resource.qrc)
# qt5_add_resources(RESOURE_FILE ${RESOURCE_SOURCE})
add_subdirectory(doodle_exe)
add_subdirectory(doodle_lib)
#add_subdirectory(doodle_server)
#add_subdirectory(plug)

#target_include_directories(core PUBLIC ${PROJECT_SOURCE_DIR}/include)
#target_include_directories(pinyin PUBLIC ${PROJECT_SOURCE_DIR}/include)
#target_include_directories(logger PUBLIC ${PROJECT_SOURCE_DIR}/include)
# target_include_directories(fileSystem PUBLIC ${CMAKE_CURRENT_LIST_DIR})
