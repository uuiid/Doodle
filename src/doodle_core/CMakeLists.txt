

# 添加资源
add_subdirectory(resource)
cmrc_add_resource_library(
        DoodleLibResource
        resource/fa-brands-400.ttf
        resource/fa-regular-400.ttf
        resource/fa-solid-900.ttf
        ${DOODLE_RE_FILES_LIST}
)
add_library(doodle_core)

# 添加库目录
target_include_directories(doodle_core
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/..
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
        )
target_sources(doodle_core
        PUBLIC
        doodle_core_fwd.h
        doodle_core.h)

# 添加子目录
add_subdirectory(pin_yin)

configure_file(configure/config.cpp.in ${CMAKE_CURRENT_LIST_DIR}/configure/config.cpp)
add_subdirectory(configure)

include(GenerateExportHeader)
generate_export_header(doodle_core)

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
        nlohmann_json nlohmann_json::nlohmann_json
        EnTT::EnTT
        spdlog::spdlog
        range-v3 range-v3-meta range-v3::meta range-v3-concepts
        cryptopp-static
)

target_compile_definitions(doodle_core
        PUBLIC
        $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=1>
        PRIVATE
        #        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_core,TYPE>,SHARED_LIBRARY>:ENTT_API_EXPORT>
        )

target_compile_options(doodle_core
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHsc>
        )
set_target_properties(doodle_core
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
