find_package(Qt5 CONFIG COMPONENTS Core REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(Boost COMPONENTS filesystem REQUIRED)

set(HEADER_FILES
        fileSystem_global.h
        src/DfileSyntem.h
        src/ftpsession.h
        )

add_library(fileSystem SHARED
        ${HEADER_FILES}
        src/DfileSyntem.cpp
        src/ftpsession.cpp
        )

target_link_libraries(fileSystem PUBLIC Qt5::Core)
target_link_libraries(fileSystem PUBLIC CURL::libcurl)
target_link_libraries(fileSystem PUBLIC ${Boost_LIBRARIES})
target_link_libraries(fileSystem PUBLIC logger)


target_compile_definitions(fileSystem PRIVATE FILESYSTEM_LIBRARY)

target_include_directories(fileSystem PUBLIC ${CMAKE_CURRENT_LIST_DIR})



install(TARGETS fileSystem DESTINATION bin)
#install(FILES ${HEADER_FILES} DESTINATION include)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:fileSystem>
        DESTINATION bin
        CONFIGURATIONS Debug)
#安装引用库
install(FILES $<TARGET_FILE:CURL::libcurl>
        DESTINATION bin)
#运行qt
install(CODE "
execute_process(
COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/bin $<TARGET_FILE:fileSystem>
)
")

#if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
#    add_custom_command(
#            TARGET fileSystem POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:fileSystem> ${CMAKE_SOURCE_DIR}/install/bin/fileSystem.dll
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:fileSystem> ${CMAKE_SOURCE_DIR}/install/bin/fileSystem.pdb
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CURL::libcurl> ${CMAKE_SOURCE_DIR}/install/bin/
#    )
#    include(BundleUtilities)
#    fixup_bundle(${CMAKE_SOURCE_DIR}/install/bin/doodle.exe  ""   "${CMAKE_FIND_ROOT_PATH}")
##    file(COPY $<TARGET_FILE:CURL::libcurl> DESTINATION ${CMAKE_SOURCE_DIR}/install/bin/libcurl-d.dll)
#else()
#    add_custom_command(
#            TARGET fileSystem POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:fileSystem> ${CMAKE_SOURCE_DIR}/install/bin/fileSystem.dll
#    )
#endif()

