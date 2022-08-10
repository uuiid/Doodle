
# 这个库是使用插件作为连接的库, 使用json rpc 客户端加载数据


add_library(doodle_tool)
target_sources(
        doodle_tool
        PUBLIC
        doodle_tool_lib.h
        doodle_tool_lib_fwd.h
)

add_subdirectory(configure)


target_link_libraries(
        doodle_tool
        PUBLIC
        doodle_core

)


target_compile_options(doodle_tool
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHs>
        )
set_target_properties(doodle_tool
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
