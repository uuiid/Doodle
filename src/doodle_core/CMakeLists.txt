

# 添加资源
add_subdirectory(resource)
cmrc_add_resource_library(
        DoodleLibResource
        resource/fa-brands-400.ttf
        resource/fa-regular-400.ttf
        resource/fa-solid-900.ttf
        ${DOODLE_RE_FILES_LIST}
)
# 添加配置

add_library(doodle_core)

# 添加库目录
target_include_directories(doodle_core
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/..
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        )

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


# 添加子目录
add_subdirectory(configure)
add_subdirectory(core)
add_subdirectory(exception)
add_subdirectory(lib_warp)
add_subdirectory(logger)
add_subdirectory(metadata)
add_subdirectory(pin_yin)
add_subdirectory(thread_pool)


target_include_directories(doodle_core
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        )
# 连接库
target_link_libraries(
        doodle_core
        PUBLIC
        date::date
        date::date-tz
        DoodleLibResource
        magic_enum::magic_enum
        Boost::locale
        Boost::contract
        #        Boost::uuid
        nlohmann_json nlohmann_json::nlohmann_json
        EnTT::EnTT
        spdlog::spdlog
        range-v3 range-v3-meta range-v3::meta range-v3-concepts
        cryptopp-static
)

target_compile_definitions(doodle_core
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_core,TYPE>,STATIC_LIBRARY>:DOODLE_CORE_STATIC_DEFINE>
        PRIVATE

        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_core,TYPE>,SHARED_LIBRARY>:doodle_core_EXPORTS>
        #        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_core,TYPE>,SHARED_LIBRARY>:ENTT_API_EXPORT>
        )

target_compile_options(doodle_core
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHsc>
        )
set_target_properties(doodle_core
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
