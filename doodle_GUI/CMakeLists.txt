find_package(Qt5 CONFIG COMPONENTS Core Widgets Sql QuickControls2 REQUIRED)

file(COPY ${PROJECT_SOURCE_DIR}/Qt-Frameless-Window-DarkStyle/DarkStyle.h
        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/resource)
file(COPY ${PROJECT_SOURCE_DIR}/Qt-Frameless-Window-DarkStyle/DarkStyle.cpp
        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/resource)
#file(COPY ${PROJECT_SOURCE_DIR}/Qt-Frameless-Window-DarkStyle/framelesswindow/framelesswindow.h
#        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/resource)
#file(COPY ${PROJECT_SOURCE_DIR}/Qt-Frameless-Window-DarkStyle/framelesswindow/framelesswindow.cpp
#        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/resource)
#file(COPY ${PROJECT_SOURCE_DIR}/Qt-Frameless-Window-DarkStyle/framelesswindow/framelesswindow.ui
#        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/resource)


set(DOODLE_SOURE_HEADER
        resource/DarkStyle.h
        src/fileClassShotWidget.h
        src/shotListWidget.h
        src/episodesListWidget.h
        src/fileTypeShotWidget.h
        src/mainWindows.h
        )
set(DOODLE_SOURE
        resource/DarkStyle.cpp
        src/fileClassShotWidget.cpp
        src/shotListWidget.cpp
        src/episodesListWidget.cpp
        src/fileTypeShotWidget.cpp
        src/mainWindows.cpp)

set(TS_FILES

        )

set(RESOURCE_SOURCE
        resource/resource.qrc
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
target_link_libraries(doodle PRIVATE Qt5::Sql)
target_link_libraries(doodle PRIVATE Qt5::Widgets)
target_link_libraries(doodle PRIVATE Qt5::QuickControls2)
target_link_libraries(doodle PRIVATE core)
target_link_libraries(doodle PRIVATE logger)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
#        COMMAND ${CMAKE_COMMAND} -E rm -r -f ${CMAKE_BINARY_DIR}/doodle_GUI
#add_custom_command(
#        TARGET doodle POST_BUILD
#        COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_BINARY_DIR}/doodle_GUI $<TARGET_FILE:doodle>
#)