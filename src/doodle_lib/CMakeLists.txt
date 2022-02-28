add_subdirectory(resource)
cmrc_add_resource_library(
        DoodleLibResource
        ${DOODLE_RE_FILES_LIST}
)
include(doodle_exe)
include(doodle_lib.cmake)

include(${PROJECT_SOURCE_DIR}/CMake/fix_mariadb_plug.cmake)

doodle_sqlpp_generate(
        OUT_LISTS_sqlpp
        LISTS_FILES
        core/000-metadatatab.sql
        core/001-usertab.sql
)
#doodle_grpc_generate(
#        OUT_LISTS_GRPC
#        LISTS_FILES
#        rpc/metadata_server.proto
#        rpc/file_system_server.proto
#        rpc/user_message.proto
#)

add_library(doodle_lib
        #        SHARED
        #        $<$<CXX_COMPILER_ID:MSVC>:SHARED>
        )
target_sources(doodle_lib PRIVATE ${DOODLELIB_SOURCE})
target_sources(doodle_lib PUBLIC ${DOODLELIB_HEADER} ${OUT_LISTS_sqlpp})

target_precompile_headers(doodle_lib
        PUBLIC [[<doodle_lib/doodle_lib_pch.h>]])

include(GenerateExportHeader)
set(MY_CUSTOM_CONTENT "")
generate_export_header(doodle_lib
        EXPORT_MACRO_NAME DOODLELIB_API
        CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)

set_target_properties(doodle_lib
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# 如果使用mysql mariadb  连接器则需要这个函数
#fix_mariadb_plug(NAME doodle_lib)


target_link_libraries(doodle_lib
        PUBLIC
        date::date
        date::date-tz
        DoodleLibResource
        magic_enum::magic_enum
        Boost::locale
        Boost::program_options
        nlohmann_json nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
        EnTT::EnTT
        spdlog::spdlog
        Catch2::Catch2
        sqlpp11::sqlite3
        sqlpp11::sqlpp11
        imgui::imgui
        range-v3 range-v3-meta range-v3::meta range-v3-concepts
        cryptopp-static
        PRIVATE
        d3d11.lib
        dxgi.lib
        Microsoft::DirectXTK
        )
target_compile_definitions(doodle_lib
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        PRIVATE
        #        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_lib,TYPE>,SHARED_LIBRARY>:ENTT_API_EXPORT>
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
#add_custom_target(genexdebug
#        COMMAND ${CMAKE_COMMAND} -E echo "$<$<STREQUAL:$<TARGET_PROPERTY:doodle_lib,TYPE>,SHARED_LIBRARY>:'is lib'>")
