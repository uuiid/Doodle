target_sources(test_main
        PRIVATE
        pinyin.cpp
        time.cpp
        bvh.cpp
        create_move.cpp
        boost_log.cpp
        range.cpp
        database_sqlite3.cpp
        entt_test.cpp
        regex.cpp
        abc.cpp
        run_process_v2.cpp
        auto_light.cpp
        xlsx_table.cpp
)

target_link_libraries(
        test_main
        PUBLIC
        Boost::log
        Boost::log_setup
        Alembic::Alembic
)

# target_compile_definitions(
# test_main
# PUBLIC
# BOOST_LOG_DYN_LINK
# BOOST_USE_WINAPI_VERSION=0x0A00
# )
doodle_link_test(test_main)
add_test(NAME test_boost_log COMMAND test_main --log_level=message --color_output=true --run_test=test_boost_log WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_test(NAME test_bvh COMMAND test_main --log_level=message --color_output=true --run_test=bvh_test WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_test(NAME test_pingyi COMMAND test_main --log_level=message --color_output=true --run_test=test_pingyi WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_test(NAME test_renges COMMAND test_main --log_level=message --color_output=true --run_test=renges WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_test(NAME tset_time COMMAND test_main --log_level=message --color_output=true --run_test=tset_time WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
add_test(NAME test_maya_abc_r COMMAND test_main --log_level=message --color_output=true --run_test=maya_abc_r WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# error:
