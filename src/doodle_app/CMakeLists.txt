add_library(
        doodle_app
        STATIC
)


target_include_directories(
        doodle_app
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/..
        ${iconfontcppheaders_SOURCE_DIR}
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
        imgui::imgui

        PRIVATE
        d3d11.lib
        dxgi.lib
)

target_compile_options(doodle_app
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHs >
        PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>: /bigobj >
        )

set_target_properties(doodle_app
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
