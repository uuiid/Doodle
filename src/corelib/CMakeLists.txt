set(core_HEADER 
        assets/assClass.h
        assets/assdepartment.h
        assets/assfilesqlinfo.h
        assets/assType.h
        assets/znchName.h

        core/coreDataManager.h
        core/coreset.h
        core/coresql.h
        core/CoreData.h
        core/Project.h

        coreOrm/doodleConfig_sqlOrm.h
        coreOrm/PathParserData_sqlOrm.h
        coreOrm/zh_ch_sqlOrm.h

        Exception/Exception.h

        exeWrap/ffmpegWrap.h

        fileArchive/fileArchive.h
        fileArchive/imageArchive.h
        fileArchive/mayaArchive.h
        fileArchive/mayaArchiveShotFbx.h
        fileArchive/moveShotA.h
        fileArchive/movieArchive.h
        fileArchive/movieEpsArchive.h
        fileArchive/ScreenshotArchive.h
        fileArchive/ueArchive.h
        fileArchive/ueSynArchive.h
        filesystem/fileSync.h
        filesystem/FileSystem.h
        filesystem/Path.h

        queueData/queueData.h
        queueData/queueManager.h

        shots/episodes.h
        shots/shot.h
        shots/shotClass.h
        shots/shotfilesqlinfo.h
        shots/ShotModifySQLDate.h
        shots/shottype.h

        sysData/synData.h

        core/PathParser.h
        threadPool/ThreadPool.h
        fileDBInfo/CommentInfo.h
        fileDBInfo/filesqlinfo.h
        fileDBInfo/pathParsing.h
        fileDBInfo/ModifySQLDate.h
        )

set(core_CPP
        assets/assClass.cpp
        assets/assdepartment.cpp
        assets/assfilesqlinfo.cpp
        assets/assType.cpp
        assets/znchName.cpp

        core/coreDataManager.cpp
        core/coreset.cpp
        core/coresql.cpp
        core/CoreData.cpp
        core/Project.cpp

        Exception/Exception.cpp
        
        exeWrap/ffmpegWrap.cpp
        
        fileArchive/fileArchive.cpp
        fileArchive/imageArchive.cpp
        fileArchive/mayaArchive.cpp
        fileArchive/mayaArchiveShotFbx.cpp
        fileArchive/moveShotA.cpp
        fileArchive/movieArchive.cpp
        fileArchive/movieEpsArchive.cpp
        fileArchive/ScreenshotArchive.cpp
        fileArchive/ueArchive.cpp
        fileArchive/ueSynArchive.cpp
        filesystem/fileSync.cpp
        filesystem/FileSystem.cpp
        filesystem/Path.cpp
        
        queueData/queueData.cpp
        queueData/queueManager.cpp

        shots/episodes.cpp
        shots/shot.cpp
        shots/shotClass.cpp
        shots/shotfilesqlinfo.cpp
        shots/ShotModifySQLDate.cpp
        shots/shottype.cpp
        
        sysData/synData.cpp
        core/PathParser.cpp
        fileDBInfo/CommentInfo.cpp
        fileDBInfo/filesqlinfo.cpp
        fileDBInfo/pathParsing.cpp
        fileDBInfo/ModifySQLDate.cpp

        )
add_library(core SHARED
${core_HEADER}
${core_CPP}
core_global.h
core_Cpp.h

)


target_link_libraries(core PUBLIC SqlppSqlite)
target_link_libraries(core PUBLIC RTTR::Core)

target_link_libraries(core PUBLIC magic_enum::magic_enum)
target_link_libraries(core PUBLIC ${Boost_LIBRARIES})
# target_link_libraries(core PUBLIC Boost::signals2)

target_link_libraries(core PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
target_link_libraries(core PRIVATE cppzmq cppzmq-static)
# target_link_libraries(core PUBLIC pugixml)
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