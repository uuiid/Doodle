

target_sources(test_main
        PRIVATE
        base_rpc.cpp
)

add_test(NAME rpc_client_1
        COMMAND test_main --log_level=message --color_output=true --run_test=rpc_client
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)