# 添加资源
add_subdirectory(resource)
cmrc_add_resource_library(
        DoodleLibResource
        ${DOODLE_RE_FILES_LIST}
        core/sql_file.sql
)

# 添加配置
add_library(doodle_core
        #        SHARED
)

target_precompile_headers(doodle_core
        PRIVATE
        doodle_core_pch.h
)
# 寻找 winreg 文件
find_path(WINREG_INCLUDE_DIRS "winreg/WinReg.hpp")

# 添加库目录
target_include_directories(doodle_core
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/..
        ${WINREG_INCLUDE_DIRS}
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)
string(TIMESTAMP DOODLE_BUILD_TIME "%Y %m %d %H %M %S" UTC)
message("build time ${DOODLE_BUILD_TIME}")
configure_file(configure/config.cpp.in ${CMAKE_CURRENT_BINARY_DIR}/configure/config.cpp)
target_sources(doodle_core
        PUBLIC
        doodle_core_fwd.h
        doodle_core.h
        doodle_core_pch.h
        doodle_macro.h

        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/configure/config.cpp
)
target_compile_definitions(doodle_core PUBLIC BOOST_FILESYSTEM_VERSION=4)

# 添加子目录
add_subdirectory(configure)
add_subdirectory(core)
add_subdirectory(exception)
add_subdirectory(json_rpc)
add_subdirectory(lib_warp)
add_subdirectory(logger)
add_subdirectory(metadata)
add_subdirectory(pin_yin)
add_subdirectory(platform)
add_subdirectory(thread_pool)
add_subdirectory(details)
add_subdirectory(time_tool)
add_subdirectory(sqlite_orm)

# 连接库
target_link_libraries(
        doodle_core
        PUBLIC
        DoodleLibResource
        magic_enum::magic_enum
        Boost::locale
        Boost::contract
        Boost::filesystem
        Boost::coroutine

        nlohmann_json nlohmann_json::nlohmann_json
        EnTT::EnTT
        spdlog::spdlog
        range-v3 range-v3-meta range-v3::meta range-v3-concepts
        cryptopp::cryptopp
        WIL::WIL
        tl::expected

        PRIVATE
        windowsapp.lib
)

target_compile_definitions(doodle_core
        PUBLIC
        WIN32_LEAN_AND_MEAN
        BOOST_PROCESS_USE_STD_FS
        BOOST_DLL_USE_STD_FS
        _WIN32_WINNT=0x0A00
        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_core,TYPE>,STATIC_LIBRARY>:DOODLE_CORE_STATIC_DEFINE>
        SPDLOG_ACTIVE_LEVEL=0
        PRIVATE

        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_core,TYPE>,SHARED_LIBRARY>:DOODLE_CORE_EXPORT>

        # $<$<STREQUAL:$<TARGET_PROPERTY:doodle_core,TYPE>,SHARED_LIBRARY>:ENTT_API_EXPORT>
)

target_compile_options(doodle_core
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:__cplusplus>
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>

        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/MP>
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)
set_target_properties(doodle_core
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
