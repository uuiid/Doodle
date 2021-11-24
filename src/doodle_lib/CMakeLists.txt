add_subdirectory(resource)
cmrc_add_resource_library(
        DoodleLibResource
        ${DOODLE_RE_FILES_LIST}
)
include(doodle_exe)
include(doodle_lib.cmake)
include(${PROJECT_SOURCE_DIR}/CMake/fix_mariadb_plug.cmake)
add_custom_target(
        create_list_doodle_cmake
        COMMAND ${CMAKE_COMMAND} -DDOODLE_INCLUDE_CMAKE=${PROJECT_SOURCE_DIR} -P list_file.cmake
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
        rpc/metadata_server.proto
        rpc/file_system_server.proto
        rpc/user_message.proto
)

add_library(doodle_lib
        SHARED
#        $<$<CXX_COMPILER_ID:MSVC>:SHARED>
        ${DOODLELIB_HEADER}
        ${DOODLELIB_SOURCE}
        ${OUT_LISTS_sqlpp}
        ${OUT_LISTS_GRPC}
        ${PROJECT_SOURCE_DIR}/stl.natvis
        external/ImGuiFileDialog/ImGuiFileDialog.cpp
        external/ImGuiFileDialog/ImGuiFileDialog.h
        external/ImGuiFileDialog/ImGuiFileDialogConfig.h
        external/service-base/ServiceBase.cpp
        external/service-base/ServiceBase.h
        external/service-base/ServiceInstaller.cpp
        external/service-base/ServiceInstaller.h
        doodle_lib.manifest
        )

target_precompile_headers(doodle_lib
        PUBLIC [[<doodle_lib/doodle_lib_pch.h>]])
        
include(GenerateExportHeader)
set(MY_CUSTOM_CONTENT "")
generate_export_header(doodle_lib
        EXPORT_MACRO_NAME DOODLELIB_API
        CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)

set_target_properties(doodle_lib
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

fix_mariadb_plug(NAME doodle_lib)


target_link_libraries(doodle_lib
        PUBLIC
        date::date
        date::date-tz
        #        SqlppSqlite
        DoodleLibResource
        # RTTR::Core
        cereal
        magic_enum::magic_enum
        Boost::locale
        Boost::filesystem
        Boost::serialization
        Boost::program_options
        nlohmann_json nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
        EnTT::EnTT
        spdlog::spdlog
        csv
        Catch2::Catch2
        SqlppMySql #这个是我们自己寻找的mysql sqlpp连接器
        imgui::imgui
        gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts gRPC::grpc++_reflection
        cryptopp-static
        d3d11.lib
        )
target_compile_definitions(doodle_lib
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        PRIVATE
        ENTT_API_EXPORT
        )
target_compile_options(doodle_lib
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHsc>
        )

target_include_directories(doodle_lib
        PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/external/csv-parser/include
        )
if (NOT RUN_COPY)
    target_compile_options(doodle_lib PRIVATE /Zi)
    target_link_options(doodle_lib PRIVATE /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF) 
endif()