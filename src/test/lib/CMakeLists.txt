target_sources(test_main
        PRIVATE
        holidaycn.cpp
        )
add_test(NAME test_holidaycn
        COMMAND test_main --log_level=message --color_output=true --run_test=test_holidaycn
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)