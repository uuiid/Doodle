find_package(Qt5 COMPONENTS Core Sql REQUIRED)

set(HEADER_FILES
    "src/convert.h"
    "pinyin_global.h"
    )

set(RESOURE_FILE
    ${PROJECT_SOURCE_DIR}/resource/pinyin/pinyin.db)

add_library(pinyin SHARED
    ${HEADER_FILES}
    "src/convert.cpp"
    )

target_link_libraries(pinyin PUBLIC Qt5::Core Qt5::Sql)
target_compile_definitions(pinyin PRIVATE PINYIN_LIBRARY)

target_include_directories(pinyin PUBLIC ${CMAKE_CURRENT_LIST_DIR})
install(TARGETS pinyin DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)
install(FILES ${RESOURE_FILE} DESTINATION resource)

