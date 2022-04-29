
# 这个库是使用插件作为连接的库, 使用json rpc 客户端加载数据


add_library(doodle_plug)
target_sources(
        doodle_plug
        PUBLIC
        doodle_plug_lib.h
        doodle_plug_lib_fwd.h
)

add_subdirectory(configure)
add_subdirectory(json_rpc)


target_link_libraries(
        doodle_plug
        PUBLIC
        doodle_core

)


target_compile_options(doodle_plug
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHsc>
        )
set_target_properties(doodle_plug
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
