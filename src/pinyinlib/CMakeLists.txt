


add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.h ${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.cpp
COMMAND ${BIN2CPP} --file=${CMAKE_CURRENT_LIST_DIR}/zhtopy.txt --output=. --headerfile=doodlePinyin.h --identifier=pinyinData
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
COMMENT "bin2cpp create pinyin lib file"
)

add_library(pinyin SHARED
${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.cpp
${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.h
)
set_target_properties(pinyin 
PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)



target_sources(pinyin
        PRIVATE
        convert.cpp
)


target_link_libraries(pinyin PRIVATE Boost::locale)
target_link_libraries(pinyin PRIVATE logger)

target_compile_definitions(pinyin 
PRIVATE PINYIN_LIBRARY NOMINMAX)

