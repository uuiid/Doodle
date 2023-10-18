
add_library(doodle_server_lib
        #        SHARED
        #        $<$<CXX_COMPILER_ID:MSVC>:SHARED>


)

add_subdirectory(core)
add_subdirectory(facet)
add_subdirectory(render_farm)


set_target_properties(doodle_server_lib
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)


target_link_libraries(doodle_server_lib
        PUBLIC
        doodle_core
        doodle_app
        date::date
        date::date-tz
        DoodleLibResource
        magic_enum::magic_enum
        Boost::locale
        Boost::contract
        Boost::url
        nlohmann_json nlohmann_json::nlohmann_json
        EnTT::EnTT
        spdlog::spdlog
        sqlpp11::sqlite3
        sqlpp11::sqlpp11
        imgui::imgui
        range-v3 range-v3-meta range-v3::meta range-v3-concepts
        cryptopp::cryptopp
        #        msgpack-cxx

        PRIVATE
        DoodleLibResourceFont
        Eigen3::Eigen
        doodle_holidaycn
        Alembic::Alembic # abc文件读写
)

target_compile_definitions(doodle_server_lib
        PUBLIC
        WIN32_LEAN_AND_MEAN
        PRIVATE
)
target_compile_options(doodle_server_lib
        PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>: /EHs>
        $<$<CXX_COMPILER_ID:MSVC>: /MP>
        $<$<CXX_COMPILER_ID:MSVC>: /bigobj>
)

target_include_directories(doodle_server_lib
        PUBLIC
        ${PROJECT_SOURCE_DIR}/src
)