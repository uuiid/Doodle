
cmrc_add_resource_library(
        DoodleLibResource
        resource/abcExport.py
        resource/icon.ico
        resource/mayaExport.py
        resource/mayaIcon.png
        resource/Ue4CraeteLevel.py
        resource/ue4Icon.png
        resource/zhtopy.txt

)
include(doodle_exe)
include(DoodleLib.cmake)

add_custom_target(
        create_list_doodle_cmake
        COMMAND ${CMAKE_COMMAND} -P ListFile.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)

doodle_sqlpp_generate(
        OUT_LISTS_sqlpp
        LISTS_FILES
        core/000-metadatatab.sql
        core/001-usertab.sql
)
doodle_grpc_generate(
        OUT_LISTS_GRPC
        LISTS_FILES
        rpc/MetadataServer.proto
        rpc/FileSystemServer.proto
        rpc/UserMessage.proto
)

add_doodle(
        NAME DoodleLib
        LISTS_FILES
        ${DOODLELIB_HEADER}
        ${DOODLELIB_SOURCE}
        ${OUT_LISTS_sqlpp}
        ${OUT_LISTS_GRPC}
        main/doodle.manifest
)
add_doodle(
        NAME DoodleExe
        IS_EXE
        ADD_WIN32
        LISTS_FILES
        ${DOODLELIB_HEADER}
        ${DOODLELIB_SOURCE}
        ${OUT_LISTS_sqlpp}
        ${OUT_LISTS_GRPC}
        main/doodle.manifest
        main/icon.ico
        main/win_exe.rc
        main/gui/main.cpp
)
add_doodle(
        NAME doodleServerExe
        IS_EXE
        ADD_WIN32
        LISTS_FILES
        ${DOODLELIB_HEADER}
        ${DOODLELIB_SOURCE}
        ${OUT_LISTS_sqlpp}
        ${OUT_LISTS_GRPC}
        main/doodle.manifest
        main/icon.ico
        main/win_exe.rc
        main/server/main.cpp
)
add_doodle(
        NAME doodle_test
        IS_EXE
        LISTS_FILES
        ${DOODLELIB_HEADER}
        ${DOODLELIB_SOURCE}
        ${OUT_LISTS_sqlpp}
        ${OUT_LISTS_GRPC}
        main/test/main.cpp
        main/icon.ico
        main/win_exe.rc
        main/test/test_core.cpp
        main/test/test_metadata.cpp
        main/test/test_server.cpp
        main/test/test_external.cpp
)
target_compile_options(doodle_test PRIVATE /bigobj)
include_directories(${PROJECT_SOURCE_DIR}/external/csv_parser/include)
include(Catch)
catch_discover_tests(doodle_test)
