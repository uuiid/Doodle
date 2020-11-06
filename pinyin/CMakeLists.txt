find_package(Qt5 CONFIG COMPONENTS Core Sql REQUIRED)

set(HEADER_FILES
    "src/convert.h"
    "pinyin_global.h"
    )

set(RESOURE_FILE
        resource/resource.qrc)

qt5_add_resources(RESOURE_FILE resource/resource.qrc)

add_library(pinyin SHARED
    ${HEADER_FILES}
    ${RESOURE_FILE}
    "src/convert.cpp"
    )



# file(COPY ${RESOURE_FILE} DESTINATION ${CMAKE_BINARY_DIR}/resource)

target_link_libraries(pinyin PUBLIC Qt5::Core Qt5::Sql)
target_link_libraries(pinyin PUBLIC logger)
target_compile_definitions(pinyin PRIVATE PINYIN_LIBRARY)

#添加导入目录
target_include_directories(pinyin PUBLIC ${CMAKE_CURRENT_LIST_DIR})

#安装基本组件
install(TARGETS pinyin
        DESTINATION bin)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:pinyin>
        DESTINATION bin
        CONFIGURATIONS Debug)

#if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
#    add_custom_command(
#            TARGET pinyin POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pinyin> ${CMAKE_SOURCE_DIR}/install/bin/pinyin.dll
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:pinyin> ${CMAKE_SOURCE_DIR}/install/bin/pinyin.pdb
#    )
#else()
#    add_custom_command(
#            TARGET pinyin POST_BUILD
#            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pinyin> ${CMAKE_SOURCE_DIR}/install/bin/pinyin.dll
#    )
#endif()
