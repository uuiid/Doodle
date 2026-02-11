add_library(doodle_lib
        #        SHARED
        #        $<$<CXX_COMPILER_ID:MSVC>:SHARED>


)

target_sources(doodle_lib
        PUBLIC
        doodle_lib_fwd.h
        doodle_lib_all.h
        doodle_lib_pch.h
)


target_precompile_headers(doodle_lib
        PRIVATE [[<doodle_lib/doodle_lib_pch.h>]])

include(GenerateExportHeader)
set(MY_CUSTOM_CONTENT "")
generate_export_header(doodle_lib
        EXPORT_MACRO_NAME DOODLELIB_API
        CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)

add_subdirectory(core)
# add_subdirectory(dem_bones)
add_subdirectory(exe_warp)
add_subdirectory(long_task)
add_subdirectory(launch)
add_subdirectory(http_method)
add_subdirectory(http_client)
# add_subdirectory(ai)

set_target_properties(doodle_lib
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 添加fbx动态库复制
add_custom_command(
        TARGET doodle_lib
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Fbx::Fbx> ${CMAKE_BINARY_DIR}/bin
)

# 如果使用mysql mariadb  连接器则需要这个函数
# fix_mariadb_plug(NAME doodle_lib)
# string(TIMESTAMP doodle_time "(%Y_y/ %m/ %d_d) + %Hh +%Mmin+ %Ss" UTC)
#
# message("doodle_time=${doodle_time}")
target_link_libraries(doodle_lib
        PUBLIC
        doodle_core
        magic_enum::magic_enum
        Boost::locale
        Boost::contract
        Boost::url
        Boost::iostreams
        Boost::process
        nlohmann_json nlohmann_json::nlohmann_json
        ${OpenCV_LIBS}
        opencv_ml opencv_dnn opencv_core opencv_flann
        EnTT::EnTT
        spdlog::spdlog
        range-v3 range-v3-meta range-v3::meta range-v3-concepts
        cryptopp::cryptopp
        #        msgpack-cxx
        #        Fbx::Fbx # fbx

        PRIVATE
        mimalloc
        netapi32.lib # NetUserGetLocalGroups
        Mpr.lib
        #        Microsoft::DirectXTK
        Eigen3::Eigen
        Alembic::Alembic # abc文件读写
        sqlite_orm::sqlite_orm
        OpenSSL::SSL
        mailio::mailio
        unofficial::bit7z::bit7z64
        # torch
        Fbx::Fbx-static
        avcpp::avcpp
        avcpp::FFmpeg
        crashpad::crashpad
        #        OpenSSL::applink
)


# 添加7zip动态库复制
add_custom_command(
        TARGET doodle_lib
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:7zip::7zip> ${CMAKE_BINARY_DIR}/bin
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
#        COMMAND ${CMAKE_COMMAND} -E echo "$<TARGET_FILE:Fbx::Fbx> $<TARGET_RUNTIME_DLLS:doodle_lib>")
