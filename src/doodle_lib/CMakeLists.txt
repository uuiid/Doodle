
cmrc_add_resource_library(
        doodle_holidaycn
        WHENCE ${holidaycn_SOURCE_DIR}
        ${holidaycn_SOURCE_DIR}/2023.json
        ${holidaycn_SOURCE_DIR}/2024.json
)


add_library(doodle_lib
        #        SHARED
        #        $<$<CXX_COMPILER_ID:MSVC>:SHARED>


)

target_sources(doodle_lib
        PUBLIC
        doodle_lib_fwd.h
        doodle_lib_all.h
        doodle_lib_pch.h
        ${OUT_LISTS_sqlpp}
)


target_precompile_headers(doodle_lib
        PRIVATE [[<doodle_lib/doodle_lib_pch.h>]])

include(GenerateExportHeader)
set(MY_CUSTOM_CONTENT "")
generate_export_header(doodle_lib
        EXPORT_MACRO_NAME DOODLELIB_API
        CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)

add_subdirectory(attendance)
add_subdirectory(core)
# add_subdirectory(dem_bones)
add_subdirectory(exe_warp)
add_subdirectory(file_warp)
add_subdirectory(long_task)
add_subdirectory(lib_warp)
add_subdirectory(platform/win)
add_subdirectory(launch)
add_subdirectory(http_method)
add_subdirectory(http_client)

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
        DoodleLibResource
        magic_enum::magic_enum
        Boost::locale
        Boost::contract
        Boost::url
        Boost::iostreams
        nlohmann_json nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
        opencv_ml opencv_dnn opencv_core opencv_flann
        EnTT::EnTT
        spdlog::spdlog
        sqlpp11::sqlite3
        sqlpp11::sqlpp11
        imgui::imgui
        range-v3 range-v3-meta range-v3::meta range-v3-concepts
        cryptopp::cryptopp
        #        msgpack-cxx

        PRIVATE
        d3d11.lib
        dxgi.lib
        netapi32.lib # NetUserGetLocalGroups
        Mpr.lib
        #        Microsoft::DirectXTK
        implot::implot
        Eigen3::Eigen
        doodle_holidaycn
        Alembic::Alembic # abc文件读写
        xlnt::xlnt #表格库
        sqlite_orm::sqlite_orm
)

target_compile_definitions(doodle_lib
        PUBLIC
        WIN32_LEAN_AND_MEAN
        PRIVATE
        #        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_lib,TYPE>,SHARED_LIBRARY>:ENTT_API_EXPORT>
)
target_compile_options(doodle_lib
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /MP>
        $<$<CXX_COMPILER_ID:MSVC>: /bigobj>
)

target_include_directories(doodle_lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)
#add_custom_target(genexdebug
#        COMMAND ${CMAKE_COMMAND} -E echo "$<$<STREQUAL:$<TARGET_PROPERTY:doodle_lib,TYPE>,SHARED_LIBRARY>:'is lib'>")
