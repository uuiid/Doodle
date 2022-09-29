add_library(
        doodle_app
        STATIC
)


target_include_directories(
        doodle_app
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/..
)


target_sources(
        doodle_app
        PUBLIC
        doodle_app_fwd.h
)

add_subdirectory(app)
add_subdirectory(configure)
add_subdirectory(gui)
add_subdirectory(platform)


# 连接库
target_link_libraries(
        doodle_app
        PUBLIC
        doodle_core
)

target_compile_definitions(doodle_app
        PRIVATE
        DOODLE_APP_EXPORT
        #        $<$<STREQUAL:$<TARGET_PROPERTY:doodle_core,TYPE>,SHARED_LIBRARY>:ENTT_API_EXPORT>
        )

target_compile_options(doodle_app
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHs >
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: /bigobj >
        )

set_target_properties(doodle_app
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
