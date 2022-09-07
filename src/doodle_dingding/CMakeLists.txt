add_library(doodle_dingding SHARED)
target_sources(
        doodle_dingding
        PUBLIC
        doodle_dingding_lib.h
        doodle_dingding_lib_fwd.h
)

add_subdirectory(configure)


target_link_libraries(
        doodle_dingding
        PUBLIC
        doodle_core

)


target_compile_options(doodle_dingding
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHs>
        )
set_target_properties(doodle_dingding
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
