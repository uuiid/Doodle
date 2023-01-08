

target_sources(test_main

        PRIVATE
        pca.cpp
)

add_test(NAME test_pca
        COMMAND test_main --log_level=message --color_output=true --run_test=test_pca
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/bin)