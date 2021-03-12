


add_custom_target(pinyin_RESOURE_FILE
COMMAND ${BIN2CPP} --file=${CMAKE_CURRENT_LIST_DIR}/zhtopy.txt --output=. --headerfile=doodlePinyin.h --identifier=pinyinData
BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.h ${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.cpp
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
SOURCES zhtopy.txt      
COMMENT "bin2cpp create pinyin lib file"
)

add_library(pinyin SHARED
${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.cpp
${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.h
)
set_target_properties(pinyin 
PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_dependencies(pinyin pinyin_RESOURE_FILE)


target_sources(pinyin
        PUBLIC
        convert.h
        pinyin_global.h
        )

target_sources(pinyin
        PRIVATE
        convert.cpp
)


target_link_libraries(pinyin PRIVATE Boost::locale)
target_link_libraries(pinyin PRIVATE logger)

target_compile_definitions(pinyin PRIVATE PINYIN_LIBRARY)

