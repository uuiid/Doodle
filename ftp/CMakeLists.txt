find_package(Qt5 CONFIG COMPONENTS Core REQUIRED)
find_package(CURL CONFIG REQUIRED)
find_package(Boost COMPONENTS filesystem REQUIRED)

set(HEADER_FILES
        ftp_global.h
        src/ftphandle.h
        src/ftpsession.h
        )

add_library(ftp SHARED
        ${HEADER_FILES}
        src/ftphandle.cpp
        src/ftpsession.cpp
        )

target_link_libraries(ftp PUBLIC Qt5::Core)
target_link_libraries(ftp PUBLIC CURL::libcurl)
target_link_libraries(ftp PUBLIC ${Boost_LIBRARIES})
target_link_libraries(ftp PUBLIC logger)


target_compile_definitions(ftp PRIVATE FTP_LIBRARY)

target_include_directories(ftp PUBLIC ${CMAKE_CURRENT_LIST_DIR})



install(TARGETS ftp DESTINATION bin)
#install(FILES ${HEADER_FILES} DESTINATION include)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:ftp>
        DESTINATION bin
        CONFIGURATIONS Debug)
#安装引用库
install(FILES $<TARGET_FILE:CURL::libcurl>
        DESTINATION bin)
#运行qt
install(CODE "
execute_process(
COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/bin $<TARGET_FILE:ftp>
)
")

#if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
#    add_custom_command(
#            TARGET ftp POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ftp> ${CMAKE_SOURCE_DIR}/install/bin/ftp.dll
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:ftp> ${CMAKE_SOURCE_DIR}/install/bin/ftp.pdb
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:CURL::libcurl> ${CMAKE_SOURCE_DIR}/install/bin/
#    )
#    include(BundleUtilities)
#    fixup_bundle(${CMAKE_SOURCE_DIR}/install/bin/doodle.exe  ""   "${CMAKE_FIND_ROOT_PATH}")
##    file(COPY $<TARGET_FILE:CURL::libcurl> DESTINATION ${CMAKE_SOURCE_DIR}/install/bin/libcurl-d.dll)
#else()
#    add_custom_command(
#            TARGET ftp POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:ftp> ${CMAKE_SOURCE_DIR}/install/bin/ftp.dll
#    )
#endif()

