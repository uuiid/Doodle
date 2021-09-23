add_subdirectory(resource)
cmrc_add_resource_library(
        DoodleLibResource
        ${DOODLE_RE_FILES_LIST}
)
include(doodle_exe)
include(DoodleLib.cmake)
include(${PROJECT_SOURCE_DIR}/CMake/fix_mariadb_plug.cmake)
add_custom_target(
        create_list_doodle_cmake
        COMMAND ${CMAKE_COMMAND} -DDOODLE_INCLUDE_CMAKE=${PROJECT_SOURCE_DIR} -P ListFile.cmake
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

add_library(DoodleLib
        SHARED
#        $<$<CXX_COMPILER_ID:MSVC>:SHARED>
        ${DOODLELIB_HEADER}
        ${DOODLELIB_SOURCE}
        ${OUT_LISTS_sqlpp}
        ${OUT_LISTS_GRPC}
        external/ImGuiFileDialog/ImGuiFileDialog.cpp
        external/ImGuiFileDialog/ImGuiFileDialog.h
        external/ImGuiFileDialog/ImGuiFileDialogConfig.h
        DoodleLib.manifest
        )
fix_mariadb_plug(NAME DoodleLib)

target_precompile_headers(DoodleLib
        PUBLIC [[<DoodleLib/DoodleLib_pch.h>]])

set(MY_CUSTOM_CONTENT "")
generate_export_header(DoodleLib
        EXPORT_MACRO_NAME DOODLELIB_API
        CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)
set_target_properties(DoodleLib
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
include(GenerateExportHeader)

target_link_libraries(DoodleLib
        PUBLIC
        date::date
        date::date-tz
        #        SqlppSqlite
        DoodleLibResource
        # RTTR::Core
        cereal
        magic_enum::magic_enum
        ${Boost_LIBRARIES}
        Boost::locale
        Boost::filesystem
        Boost::serialization
        nlohmann_json nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
        #        EnTT::EnTT
        #        Boost::locale
        #        Boost::filesystem
        #        Boost::log
        spdlog::spdlog
        #        $<IF:$<CONFIG:Debug>,${WX_based},${WX_base}>
        #        $<IF:$<CONFIG:Debug>,${WX_cored},${WX_core}>
        #        $<IF:$<CONFIG:Debug>,${WX_advd},${WX_adv}>
        csv
        Catch2::Catch2
        SqlppMySql #这个是我们自己寻找的mysql sqlpp连接器
        #        cppzmq
        #        cppzmq-static
        imgui::imgui
        gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts gRPC::grpc++_reflection
        cryptopp-static
        d3d11.lib
        )
target_compile_definitions(DoodleLib
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        )
target_compile_options(DoodleLib
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHsc>
        )

target_include_directories(DoodleLib
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/external/csv-parser/include
        )

