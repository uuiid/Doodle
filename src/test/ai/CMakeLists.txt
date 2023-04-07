#find_package(Torch)
message("TORCH_LIBRARIES=${TORCH_LIBRARIES}")

if (TARGET torch)
    add_executable(
            test_ai
            EXCLUDE_FROM_ALL
            ai_main.cpp
    )
    target_link_libraries(
            test_ai
            PUBLIC
            Boost::log
            Boost::log_setup
            "${TORCH_LIBRARIES}"
    )
    doodle_link_test(test_ai)
endif ()

# add_test(NAME test_ai_exe COMMAND test_ai --logger=HRF,all --color_output=true WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)