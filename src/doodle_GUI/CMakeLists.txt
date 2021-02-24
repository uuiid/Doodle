
set(RESOURCE_SOURCE
        resource.qrc
        style.qrc
)

qt5_add_resources(RESOURE_FILE ${RESOURCE_SOURCE})

add_executable(doodle WIN32 ${RESOURE_FILE}

main.cpp
doodle_global.h
core_cpp_qt.h
)
#
# add_executable(doodle WIN32
#         doodle_global.h
#         main.cpp
#         ${TS_FILES}
#         ${RESOURE_FILE}
#         )

add_subdirectory(source)

target_include_directories(doodle PRIVATE ${BOOST_INCLUDE_DIRS})

target_link_libraries(doodle PRIVATE Qt5::Core)
target_link_libraries(doodle PRIVATE Qt5::Widgets)
target_link_libraries(doodle PRIVATE Qt5::QuickControls2)

target_link_libraries(doodle PUBLIC ${Boost_LIBRARIES})
target_link_libraries(doodle PRIVATE core)
target_link_libraries(doodle PRIVATE logger)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

#安装基本组件
install(TARGETS doodle
        DESTINATION bin)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:doodle>
        DESTINATION bin
        CONFIGURATIONS Debug)
        
if(RUN_QT_TOOL)
        #运行qt
        install(CODE "
        execute_process(
        COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/bin $<TARGET_FILE:doodle>
        )
        ")  
endif(RUN_QT_TOOL)

