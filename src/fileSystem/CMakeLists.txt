# # find_package(Qt5 CONFIG COMPONENTS Core REQUIRED)
# # find_package(CURL CONFIG REQUIRED)
# # find_package(Boost COMPONENTS filesystem REQUIRED)
# # find_package(magic_enum CONFIG REQUIRED)

# set(HEADER_FILES
#         fileSystem_global.h
#         system/DfileSyntem.h
#         system/ftpsession.h
#         system/Path.h
#         )

# add_library(fileSystem SHARED
#         ${HEADER_FILES}

#         system/DfileSyntem.cpp
#         system/ftpsession.cpp
#         system/Path.cpp
#         fileSystem_cpp.h
#         )

# target_link_libraries(fileSystem PRIVATE Qt5::Core)
# target_link_libraries(fileSystem PRIVATE ${Boost_LIBRARIES})

# #私有连接
# target_link_libraries(fileSystem PRIVATE CURL::libcurl)
# target_link_libraries(fileSystem PRIVATE magic_enum::magic_enum)
# target_link_libraries(fileSystem PRIVATE logger)
# target_link_libraries(fileSystem PRIVATE date::date date::date-tz)
# target_link_libraries(fileSystem PRIVATE cppzmq cppzmq-static)
# #编译宏
# target_compile_definitions(fileSystem PRIVATE _WIN32_WINNT=0x0A00)
# target_compile_definitions(fileSystem PRIVATE FILESYSTEM_LIBRARY)


# install(TARGETS fileSystem DESTINATION bin)
# #install(FILES ${HEADER_FILES} DESTINATION include)
# #安装调试文件
# install(FILES $<TARGET_PDB_FILE:fileSystem>
#         DESTINATION bin
#         CONFIGURATIONS Debug)
# #安装引用库
# install(FILES $<TARGET_FILE:CURL::libcurl>
#         DESTINATION bin)

# if(RUN_QT_TOOL)
#         #运行qt
#         install(CODE "
#         execute_process(
#         COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/bin $<TARGET_FILE:fileSystem>
#         )
#         ")  
# endif(RUN_QT_TOOL)


