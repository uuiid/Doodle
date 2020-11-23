find_package(Qt5 CONFIG COMPONENTS Core Widgets QuickControls2 REQUIRED)
find_package(Boost COMPONENTS regex REQUIRED)

#file(COPY ${PROJECT_SOURCE_DIR}/Qt-Frameless-Window-DarkStyle/framelesswindow/framelesswindow.h
#        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/resource)
#file(COPY ${PROJECT_SOURCE_DIR}/Qt-Frameless-Window-DarkStyle/framelesswindow/framelesswindow.cpp
#        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/resource)
#file(COPY ${PROJECT_SOURCE_DIR}/Qt-Frameless-Window-DarkStyle/framelesswindow/framelesswindow.ui
#        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/resource)


set(DOODLE_SOURE_HEADER
        
        src/ShotClassWidget.h
        src/shotListWidget.h
        src/shotEpsListWidget.h
        src/ShotTypeWidget.h
        src/shotTableWidget.h
        src/ProjectWidget.h
        src/shotEpsListModel.h
        src/mainWindows.h
        src/ShotClassModel.h
        src/ShotTypeModel.h
        src/shotListModel.h
        src/shotTableModel.h
        src/assDepModel.h
        src/AssDepWidget.h
        src/assClassWidget.h
        src/assTableModel.h
        src/AssTypeModel.h
        src/assClassModel.h
        src/AssTypeWidget.h
        src/assTableWidght.h
        src/settingWidget.h
        src/systemTray.h
        src/speedProgressWidget.h
        src/Toolkit.h
        src/assSortfilterModel.h
        src/updataManager.h 
        src/assWidget.h
        src/shotWidget.h
        )

set(DOODLE_SOURE
        
        src/ShotClassWidget.cpp
        src/shotListWidget.cpp
        src/shotEpsListWidget.cpp
        src/ShotTypeWidget.cpp
        src/shotTableWidget.cpp
        src/mainWindows.cpp
        src/ProjectWidget.cpp
        src/shotEpsListModel.cpp
        src/ShotClassModel.cpp
        src/ShotTypeModel.cpp
        src/shotListModel.cpp
        src/shotTableModel.cpp
        src/assDepModel.cpp
        src/AssDepWidget.cpp
        src/assClassModel.cpp
        src/AssTypeModel.cpp
        src/assTableModel.cpp
        src/assClassWidget.cpp
        src/AssTypeWidget.cpp
        src/assTableWidght.cpp
        src/settingWidget.cpp
        src/systemTray.cpp
        src/speedProgressWidget.cpp
        src/Toolkit.cpp
        src/assSortfilterModel.cpp
        src/updataManager.cpp 
        src/assWidget.cpp
        src/shotWidget.cpp
        )

set(TS_FILES
        )


set(RESOURCE_SOURCE
        resource.qrc
        style.qrc
)

qt5_add_resources(RESOURE_FILE ${RESOURCE_SOURCE})

add_executable(doodle
        doodle_global.h
        ${DOODLE_SOURE_HEADER}
        ${DOODLE_SOURE}
        main.cpp
        ${TS_FILES}
        ${RESOURE_FILE}
        )

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
#运行qt
install(CODE "
execute_process(
COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/bin $<TARGET_FILE:doodle>
)
")
