find_package(Qt5 COMPONENTS Core Sql REQUIRED)

set(HEADER_FILES
    "src/convert.h"
    "pinyin_global.h"
    )

set(RESOURE_FILE
    ${PROJECT_SOURCE_DIR}/resource/resource.qrc)

qt5_add_resources(RESOURE_FILE ${PROJECT_SOURCE_DIR}/resource/resource.qrc)

add_library(pinyin SHARED
    ${HEADER_FILES}
    ${RESOURE_FILE}
    "src/convert.cpp"
    )



# file(COPY ${RESOURE_FILE} DESTINATION ${CMAKE_BINARY_DIR}/resource)

target_link_libraries(pinyin PUBLIC Qt5::Core Qt5::Sql)
target_compile_definitions(pinyin PRIVATE PINYIN_LIBRARY)

#添加导入目录
target_include_directories(pinyin PUBLIC ${CMAKE_CURRENT_LIST_DIR})
install(TARGETS pinyin DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)
install(FILES ${RESOURE_FILE} DESTINATION resource)

