find_package(Qt5 CONFIG COMPONENTS Core Widgets Sql REQUIRED)

set(DOODLE_SOURE_HEADER
    src/episodesListWidget.h
    src/mainWindows.h
)
set(DOODLE_SOURE
    src/episodesListWidget.cpp
    src/mainWindows.cpp
)

set(QM_FILES

)

add_executable(doodle
    doodle_global.h
    ${DOODLE_SOURE_HEADER}
    ${DOODLE_SOURE}    
    main.cpp
    ${TS_FILES}
)

target_link_libraries(doodle PRIVATE Qt5::Core)
target_link_libraries(doodle PRIVATE Qt5::Sql)
target_link_libraries(doodle PRIVATE Qt5::Widgets)
target_link_libraries(doodle PRIVATE core)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})