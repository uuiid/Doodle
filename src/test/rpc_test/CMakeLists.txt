

target_sources(test_main
        PRIVATE
        base_rpc.cpp
        base_server.cpp
)

add_test(NAME rpc_1
        COMMAND powershell
        [[Start-Process -FilePath $<TARGET_FILE:test_main>
        -ArgumentList "--log_level=message,--color_output=true,--run_test=rpc_server/base";
        Start-Process -FilePath $<TARGET_FILE:test_main>
        -ArgumentList "--log_level=message,--color_output=true,--run_test=rpc_client/base";]]

        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# test_main --log_level=message --color_output=true --run_test=rpc_client/base
# add_custom_target(rpc_1_server
# COMMAND test_main --log_level=message --color_output=true --run_test=rpc_server/base
# WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# set_tests_properties(rpc_1_server
# PROPERTIES
# FIXTURES_SETUP my-fixture
# )

# set_tests_properties(
# rpc_1_client
# PROPERTIES
# FIXTURES_REQUIRED my-fixture
# )