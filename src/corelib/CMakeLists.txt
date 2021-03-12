set(core_HEADER 
        assets/assClass.h
        assets/assdepartment.h
        assets/assfilesqlinfo.h
        assets/znchName.h

        core/coreDataManager.h
        core/coreset.h
        core/coresql.h
        core/CoreData.h
        core/Project.h

        coreOrm/doodleConfig_sqlOrm.h
        coreOrm/PathParserData_sqlOrm.h
        coreOrm/zh_ch_sqlOrm.h
        coreOrm/class_alias_sqlOrm.h

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
        fileDBInfo/pathParsing.h
        fileDBInfo/ModifySQLDate.h
        core/FileInfo.h

        assets/assType.h
        FileWarp/MayaFile.h
        )

set(core_CPP
        assets/assClass.cpp
        assets/assdepartment.cpp
        assets/assfilesqlinfo.cpp
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
        fileDBInfo/pathParsing.cpp
        fileDBInfo/ModifySQLDate.cpp

        core/FileInfo.cpp
        assets/assType.cpp
        FileWarp/MayaFile.cpp

        )
add_library(core SHARED
${core_HEADER}
${core_CPP}
core_global.h
core_Cpp.h

)
set_target_properties(core 
PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


target_link_libraries(core PUBLIC SqlppSqlite)
target_link_libraries(core PUBLIC RTTR::Core)

target_link_libraries(core PUBLIC magic_enum::magic_enum)
target_link_libraries(core PUBLIC ${Boost_LIBRARIES})


target_link_libraries(core PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
target_link_libraries(core PUBLIC pinyin)
target_link_libraries(core PUBLIC logger)

target_compile_definitions(core PRIVATE CORE_LIBRARY)
target_compile_definitions(core PUBLIC DOODLE_QT)

target_include_directories(core PUBLIC ${CMAKE_CURRENT_LIST_DIR})
qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
