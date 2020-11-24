find_package(Qt5 CONFIG COMPONENTS Core  REQUIRED)
find_package(Sqlpp11 REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)


list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/CMake)

find_package(SqlppMySql REQUIRED)

find_package(FFMPEG  REQUIRED )
find_package(Boost COMPONENTS filesystem regex REQUIRED)
find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

#execute_process(
#        COMMAND "C:/Program Files (x86)/bin2cpp 2.3.0/bin/bin2cpp.exe" --file=src/resource/_GlobalSettings.xml --output=src/resource/ --headerfile=freeSynGlob.h --identifier=freeSynGlob
#        COMMAND "C:/Program Files (x86)/bin2cpp 2.3.0/bin/bin2cpp.exe" --file=src/resource/_GlobalSettings.xml --output=src/resource/ --headerfile=freeSynGlob.h --identifier=freeSynGlob
#        WORKING_DIRECTORY
#)



#这个是拎一个ORM库
set(ORM_HEADER
        src/coreOrm/project_sqlOrm.h
        src/coreOrm/user_sqlOrm.h
        src/coreOrm/episodes_sqlOrm.h
        src/coreOrm/configure_sqlOrm.h
        src/coreOrm/assdepartment_sqlOrm.h
        src/coreOrm/shots_sqlOrm.h
        src/coreOrm/assclass_sqlOrm.h
        src/coreOrm/shotclass_sqlOrm.h
        src/coreOrm/synfile_sqlOrm.h
        src/coreOrm/asstype_sqlOrm.h
        src/coreOrm/znch_sqlOrm.h
        src/coreOrm/shottype_sqlOrm.h
        src/coreOrm/basefile_sqlOrm.h
        )

#这个是和MySQL交互的头文件
set(CORE_SQL_HEADER
        core_global.h
        src/filesqlinfo.h
        src/coresql.h
        src/coreset.h
        src/shotfilesqlinfo.h
        src/assfilesqlinfo.h
        src/episodes.h
        src/shot.h
        src/shotClass.h
        src/shottype.h
        src/assClass.h
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
        src/shotClass.cpp
        src/shottype.cpp
        src/assClass.cpp
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
        src/mayaArchiveShotFbx.h
        src/ffmpegWrap.h
        src/ueSynArchive.h
        src/moveShotA.h
        src/assType.h
        src/assdepartment.h
        src/coreDataManager.h

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
        src/freeSynWrap.cpp
        src/mayaArchiveShotFbx.cpp
        src/ffmpegWrap.cpp
        src/assdepartment.cpp
        src/assType.cpp
        src/moveShotA.cpp
        src/ueSynArchive.cpp
        src/coreDataManager.cpp
        src/movieEpsArchive.cpp src/movieEpsArchive.h)

set(RESOURCE_SOURCE
        core_resource.qrc)
qt5_add_resources(RESOURE_FILE ${RESOURCE_SOURCE})
add_library(core SHARED
        src/doException/doException.h
        src/doException/doException.cpp
        core_doQt.h
        ${CORE_SQL_HEADER}
        ${CORE_SQL_SOURCE}
        ${CORE_ARCHIVE_HEADER}
        ${CORE_ARCHIVE_SOURCE}
        ${RESOURE_FILE}
        )

target_link_libraries(core PUBLIC Qt5::Core)
target_link_libraries(core PUBLIC SqlppMySql)
target_link_libraries(core PUBLIC magic_enum::magic_enum)
target_link_libraries(core PUBLIC ${Boost_LIBRARIES})
#target_link_libraries(core PUBLIC jsoncpp_lib jsoncpp_object)
target_link_libraries(core PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
target_link_libraries(core PUBLIC pugixml)
target_link_libraries(core PUBLIC pinyin)
target_link_libraries(core PUBLIC ftp)
target_link_libraries(core PUBLIC logger)

target_compile_definitions(core PRIVATE CORE_LIBRARY)
target_compile_definitions(core PUBLIC DOODLE_QT)
qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_LIST_DIR})

#安装基本组件
install(TARGETS core
        DESTINATION bin)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:core>
        DESTINATION bin
        CONFIGURATIONS Debug)
#运行qt
install(CODE "
execute_process(
COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/bin ${CMAKE_INSTALL_PREFIX}/bin/core.dll
)
")
