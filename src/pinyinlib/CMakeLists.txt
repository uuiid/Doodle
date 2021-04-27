cmrc_add_resource_library(
        PinyinResource
        zhtopy.txt

)
add_library(pinyin SHARED)
set_target_properties(pinyin
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


target_sources(pinyin
        PRIVATE
        convert.cpp
        )

target_link_libraries(pinyin
        PRIVATE
        Boost::locale
        logger
        PinyinResource)

target_compile_definitions(pinyin
        PRIVATE PINYIN_LIBRARY NOMINMAX)