
cmrc_add_resource_library(
        doodle_holidaycn
        WHENCE ${holidaycn_SOURCE_DIR}
        ${holidaycn_SOURCE_DIR}/2022.json
        ${holidaycn_SOURCE_DIR}/2023.json
)


include(doodle_exe)
include(doodle_lib.cmake)
add_library(doodle_lib
        #        SHARED
        #        $<$<CXX_COMPILER_ID:MSVC>:SHARED>
        )

target_sources(doodle_lib PRIVATE ${DOODLELIB_SOURCE})
target_sources(doodle_lib PUBLIC ${DOODLELIB_HEADER} ${OUT_LISTS_sqlpp})
target_precompile_headers(doodle_lib
        PRIVATE [[<doodle_lib/doodle_lib_pch.h>]])

include(GenerateExportHeader)
set(MY_CUSTOM_CONTENT "")
generate_export_header(doodle_lib
        EXPORT_MACRO_NAME DOODLELIB_API
        CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)

add_subdirectory(app)
add_subdirectory(attendance)
add_subdirectory(json_rpc)
add_subdirectory(lib_warp)
add_subdirectory(platform/win)
add_subdirectory(gui)

set_target_properties(doodle_lib
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 如果使用mysql mariadb  连接器则需要这个函数
# fix_mariadb_plug(NAME doodle_lib)
# string(TIMESTAMP doodle_time "(%Y_y/ %m/ %d_d) + %Hh +%Mmin+ %Ss" UTC)
#
# message("doodle_time=${doodle_time}")
target_link_libraries(doodle_lib
        PUBLIC
        doodle_core
        doodle_app
        date::date
        date::date-tz
        DoodleLibResource
        magic_enum::magic_enum
        Boost::locale
        Boost::program_options
        Boost::contract
        nlohmann_json nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
        EnTT::EnTT
        spdlog::spdlog
        sqlpp11::sqlite3
        sqlpp11::sqlpp11
        imgui::imgui
        range-v3 range-v3-meta range-v3::meta range-v3-concepts
        cryptopp-static

        PRIVATE
        d3d11.lib
        dxgi.lib
        Microsoft::DirectXTK
        implot::implot
        doodle_dingding
        DoodleLibResourceFont
        Eigen3::Eigen
        doodle_holidaycn

        )

target_compile_definitions(doodle_lib
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        WIN32_LEAN_AND_MEAN
        PRIVATE
        #        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_lib,TYPE>,SHARED_LIBRARY>:ENTT_API_EXPORT>
        )
target_compile_options(doodle_lib
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHs>
        )

target_include_directories(doodle_lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/external/csv-parser/include
        )
#add_custom_target(genexdebug
#        COMMAND ${CMAKE_COMMAND} -E echo "$<$<STREQUAL:$<TARGET_PROPERTY:doodle_lib,TYPE>,SHARED_LIBRARY>:'is lib'>")
