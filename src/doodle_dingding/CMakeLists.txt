add_library(doodle_dingding SHARED)
target_sources(
        doodle_dingding
        PUBLIC
        doodle_dingding.h
        doodle_dingding_fwd.h
)
add_subdirectory(client)
add_subdirectory(configure)
add_subdirectory(fmt_lib)

target_include_directories(
        doodle_dingding
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/..
)
target_link_libraries(
        doodle_dingding
        PUBLIC
        doodle_core
        OpenSSL::SSL
        OpenSSL::Crypto
)
target_compile_definitions(
        doodle_dingding
        PRIVATE
        DOODLE_DINGDING_EXPORTS
)

target_compile_options(doodle_dingding
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHs>
        $<$<CXX_COMPILER_ID:MSVC>: /bigobj>
        )
set_target_properties(doodle_dingding
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
