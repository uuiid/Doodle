find_package(Qt5 CONFIG COMPONENTS Core Sql Xml REQUIRED)
find_package(Sqlpp11 REQUIRED)

find_package(magic_enum CONFIG REQUIRED)
#由于这个库只有一个文件,  所以直接复制过的了事
file(COPY ${PROJECT_SOURCE_DIR}/sql-builder/sql.h
        DESTINATION ${CMAKE_CURRENT_LIST_DIR}/src/sql_builder)

#这个是拎一个ORM库  还没开始用预备着
set(ORM_HEADER
        src/coreOrm/assclass_sqlOrm.h
        src/coreOrm/basefile_sqlOrm.h
        src/coreOrm/configure_sqlOrm.h
        src/coreOrm/episodes_sqlOrm.h
        src/coreOrm/fileclass_sqlOrm.h
        src/coreOrm/filetype_sqlOrm.h
        src/coreOrm/mainshot_sqlOrm.h
        src/coreOrm/shot_sqlOrm.h
        src/coreOrm/znch_sqlOrm.h
        )

#这个是和MySQL交互的头文件
set(CORE_SQL_HEADER
        core_global.h
        src/sql_builder/sql.h
        src/filesqlinfo.h
        src/coresql.h
        src/coreset.h
        src/shotfilesqlinfo.h
        src/assfilesqlinfo.h
        src/episodes.h
        src/shot.h
        src/fileclass.h
        src/filetype.h
        src/asstype.h
        src/coresqldata.h
        src/znchName.h
        )
#这个是和MySQL交互的实现文件
set(CORE_SQL_SOURCE
        src/filesqlinfo.cpp
        src/coresql.cpp
        src/coreset.cpp
        src/shotfilesqlinfo.cpp
        src/assfilesqlinfo.cpp
        src/episodes.cpp
        src/shot.cpp
        src/fileclass.cpp
        src/filetype.cpp
        src/asstype.cpp
        src/coresqldata.cpp
        src/znchName.cpp
        )

#这个是和文件服务器交互的头文件
set(CORE_ARCHIVE_HEADER
        src/archiveFactory.h
        src/fileArchive.h
        src/mayaArchive.h
        src/ueArchive.h
        src/movieArchive.h
        src/imageArchive.h
        src/imageSequenArchive.h
        src/freeSynWrap.h
        )
#这个是和文件服务器交互的实现文件
set(CORE_ARCHIVE_SOURCE
        src/archiveFactory.cpp
        src/fileArchive.cpp
        src/mayaArchive.cpp
        src/ueArchive.cpp
        src/movieArchive.cpp
        src/imageArchive.cpp
        src/imageSequenArchive.cpp
        src/freeSynWrap.cpp)

set(RESOURCE_SOURCE
        resource/resource.qrc)
qt5_add_resources(RESOURE_FILE ${RESOURCE_SOURCE})
add_library(core SHARED
        src/doException/doException.h
        src/doException/doException.cpp
        ${CORE_SQL_HEADER}
        ${CORE_SQL_SOURCE}
        ${CORE_ARCHIVE_HEADER}
        ${CORE_ARCHIVE_SOURCE}
        ${RESOURE_FILE}
        )

target_link_libraries(core PUBLIC Qt5::Sql)
target_link_libraries(core PUBLIC Qt5::Core)
target_link_libraries(core PUBLIC Qt5::Xml)
target_link_libraries(core PUBLIC magic_enum::magic_enum)
target_link_libraries(core PUBLIC pinyin)
target_link_libraries(core PUBLIC ftp)
target_link_libraries(core PUBLIC logger)

target_compile_definitions(core PRIVATE CORE_LIBRARY)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS core DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)
