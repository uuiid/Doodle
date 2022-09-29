add_executable(
        test_json_rpc
        EXCLUDE_FROM_ALL
        base_rpc.cpp
)
doodle_link_test(test_json_rpc)
add_test(NAME test_json_rpc_exe COMMAND test_json_rpc --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
