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
        FileWarp/ImageSequence.cpp
        FileWarp/VideoSequence.cpp
)
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
        threadPool/LongTerm.h
        fileDBInfo/CommentInfo.h
        fileDBInfo/pathParsing.h
        fileDBInfo/ModifySQLDate.h
        core/FileInfo.h

        assets/assType.h
        FileWarp/MayaFile.h
        libWarp/BoostUuidWarp.h
        FileWarp/ImageSequence.h
        FileWarp/VideoSequence.h
        )


add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportFbx.h 
                          ${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportFbx.cpp
COMMAND ${BIN2CPP} --file=${CMAKE_CURRENT_LIST_DIR}/resource/mayaExport.py --output=. --headerfile=CoreResourceMayaExportFbx.h --identifier=CoreResourceMayaExportFbx --override 
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
DEPENDS resource/mayaExport.py
COMMENT "bin2cpp create pinyin lib file"
)
add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportAbc.h 
                          ${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportAbc.cpp
COMMAND ${BIN2CPP} --file=${CMAKE_CURRENT_LIST_DIR}/resource/abcExport.py --output=. --headerfile=CoreResourceMayaExportAbc.h --identifier=CoreResourceMayaExportAbc --override 
WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
DEPENDS resource/abcExport.py
COMMENT "bin2cpp create pinyin lib file"
)

add_library(core SHARED
${core_HEADER}
${core_CPP}
${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportFbx.h 
${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportFbx.cpp
${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportAbc.h 
${CMAKE_CURRENT_BINARY_DIR}/CoreResourceMayaExportAbc.cpp
core_global.h
core_Cpp.h
)



set_target_properties(core 
PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#设置导出符号
include(GenerateExportHeader)
set(MY_CUSTOM_CONTENT "")
generate_export_header(core
EXPORT_MACRO_NAME CORE_API
CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)
#预编译头文件
target_precompile_headers(core
PUBLIC
<filesystem>
<variant>
<string>
<memory>
<chrono>
<map>
<iostream>
<regex>
<fstream>
<mutex>
<condition_variable>
<future>
<vector>
<unordered_set>
<queue>
<functional>
<stdexcept>
)

# find_package(Freetype REQUIRED)
# set(HARFBUZZ_ROOT "F:/vcpkg/installed/x64-windows" CACHE PATH "harfbuzz install root dir")
# find_path(HARFBUZZ_INCLUDE_DIRS
# NAMES hb-ft.h
# PATH_SUFFIXES harfbuzz
# HINTS ${HARFBUZZ_ROOT}/include)
# if(CMAKE_BUILD_TYPE MATCHES "Debug" )
# message("is debug")
# find_library(HARFBUZZ_LIBRARIES
#     NAMES harfbuzz
#     HINTS ${HARFBUZZ_ROOT}/debug/lib)
# else()
# message("not is debug")
# find_library(HARFBUZZ_LIBRARIES
#     NAMES harfbuzz
#     HINTS ${HARFBUZZ_ROOT}/lib)
# endif()
# find_package_handle_standard_args(HARFBUZZ
#     DEFAULT_MSG HARFBUZZ_LIBRARIES HARFBUZZ_INCLUDE_DIRS)

target_link_libraries(core PUBLIC 
SqlppSqlite
# RTTR::Core
magic_enum::magic_enum
${Boost_LIBRARIES}
nlohmann_json nlohmann_json::nlohmann_json
pinyin
logger
${OpenCV_LIBS}
)


target_compile_definitions(core PRIVATE CORE_LIBRARY core_EXPORT)
target_compile_definitions(core PUBLIC DOODLE_QT NOMINMAX)


target_include_directories(core PUBLIC 
${CMAKE_CURRENT_LIST_DIR}
${CMAKE_CURRENT_BINARY_DIR}
PRIVATE
${OpenCV_INCLUDE_DIRS}
)

