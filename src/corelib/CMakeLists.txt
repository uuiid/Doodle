add_library(core SHARED)


target_sources(core
 PUBLIC
core_global.h
core_Cpp.h)

add_subdirectory(assets)
add_subdirectory(core)
add_subdirectory(coreOrm)
add_subdirectory(Exception)
add_subdirectory(exeWrap)
add_subdirectory(fileArchive)
add_subdirectory(fileDBInfo)
add_subdirectory(filesystem)
add_subdirectory(queueData)
add_subdirectory(shots)
add_subdirectory(sysData)

target_link_libraries(core PUBLIC Qt5::Core)
target_link_libraries(core PUBLIC SqlppMySql)
target_link_libraries(core PUBLIC RTTR::Core)

target_link_libraries(core PUBLIC magic_enum::magic_enum)
target_link_libraries(core PUBLIC ${Boost_LIBRARIES})
# target_link_libraries(core PUBLIC Boost::signals2)

target_link_libraries(core PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
target_link_libraries(core PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
target_link_libraries(core PUBLIC pugixml)
target_link_libraries(core PUBLIC pinyin)
target_link_libraries(core PUBLIC logger)

target_compile_definitions(core PRIVATE CORE_LIBRARY)
target_compile_definitions(core PUBLIC DOODLE_QT)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_LIST_DIR})
qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

#---------------------------------------------------安装----------------------------------------------------------------
#安装基本组件
install(TARGETS core
        DESTINATION bin)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:core>
        DESTINATION bin
        CONFIGURATIONS Debug)
        
if(RUN_QT_TOOL)
        #运行qt
        install(CODE "
        execute_process(
        COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/bin ${CMAKE_INSTALL_PREFIX}/bin/core.dll
        )
        ")
endif(RUN_QT_TOOL)