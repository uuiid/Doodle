set(RESOURE_FILE
        ${CMAKE_CURRENT_LIST_DIR}/resource/resource.qrc)

qt5_add_resources(RESOURE_FILE ${CMAKE_CURRENT_LIST_DIR}/resource/resource.qrc)

add_library(pinyin SHARED ${RESOURE_FILE})

target_sources(pinyin
        PUBLIC
        convert.h
        pinyin_global.h
        )
target_sources(pinyin
        PRIVATE
        convert.cpp
)


target_link_libraries(pinyin PUBLIC Qt5::Core Qt5::Sql)
target_link_libraries(pinyin PUBLIC SqlppSqlite)
target_link_libraries(pinyin PUBLIC logger)
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
