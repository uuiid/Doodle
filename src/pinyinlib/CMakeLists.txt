


add_library(pinyin SHARED)
add_custom_target(pinyin_RESOURE_FILE
        COMMAND ${BIN2CPP} --file=${CMAKE_CURRENT_LIST_DIR}/zhtopy.txt --output=${CMAKE_CURRENT_BINARY_DIR} --headerfile=doodlePinyin.h --identifier=pinyinData
        BYPRODUCTS ${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.h ${CMAKE_CURRENT_BINARY_DIR}/doodlePinyin.cpp

        )
file(GLOB pinyin_FILES_HANDER CONFIGURE_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/*.h)
file(GLOB pinyin_FILES_CPP CONFIGURE_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/*.cpp)

target_sources(pinyin
        PUBLIC
        convert.h
        pinyin_global.h
        ${pinyin_FILES_HANDER}
        )

target_sources(pinyin
        PRIVATE
        convert.cpp
        ${pinyin_FILES_CPP}
)


target_link_libraries(pinyin PRIVATE Boost::locale)
target_link_libraries(pinyin PRIVATE logger)

target_compile_definitions(pinyin PRIVATE PINYIN_LIBRARY)


#安装基本组件
install(TARGETS pinyin
        DESTINATION bin)

#安装调试文件
install(FILES $<TARGET_PDB_FILE:pinyin>
        DESTINATION bin
        CONFIGURATIONS Debug)

if(RUN_QT_TOOL)
        #运行qt
        install(CODE "
        execute_process(
        COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/bin $<TARGET_FILE:pinyin>
        )
        ")  
endif(RUN_QT_TOOL)
