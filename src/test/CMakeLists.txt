add_executable(
        doodle_kitsu_supplement_test
        test_main.cpp
        kitsu_supplement_test.cpp
        test_reg.cpp
        ${PROJECT_SOURCE_DIR}/src/DoodleExe.manifest
        ${PROJECT_SOURCE_DIR}/src/icon.ico
        ${PROJECT_SOURCE_DIR}/src/win_exe.rc
)

set_target_properties(doodle_kitsu_supplement_test
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

target_include_directories(doodle_kitsu_supplement_test
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/src/exe_gui)

target_link_libraries(doodle_kitsu_supplement_test
        PUBLIC
        mimalloc
        doodle_lib
        PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        TBB::tbb
)

target_compile_definitions(doodle_kitsu_supplement_test
        PUBLIC
        CRTDBG_MAP_ALLOC
)

target_compile_options(doodle_kitsu_supplement_test PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
        $<$<CXX_COMPILER_ID:MSVC>:/MP>)

find_program(
        crashpad_handler_exe
        NAMES crashpad_handler.exe
        HINTS
        $<$<CONFIG:Debug>:${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/tools>
        $<$<CONFIG:Release>:${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/tools>
)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_custom_command(
                TARGET doodle_kitsu_supplement_test
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE_DIR:doodle_kitsu_supplement_test>/zlibd1.dll $<TARGET_FILE_DIR:doodle_kitsu_supplement_test>/zlib1.dll
                COMMAND ${CMAKE_COMMAND} -E copy_if_different ${crashpad_handler_exe} $<TARGET_FILE_DIR:doodle_kitsu_supplement_test>
        )
endif()

add_custom_command(
        TARGET doodle_kitsu_supplement_test
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/bin/onnxruntime_providers_cuda.dll,${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/onnxruntime_providers_cuda.dll>"
                "$<TARGET_FILE_DIR:doodle_kitsu_supplement_test>"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "$<IF:$<CONFIG:Debug>,${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/debug/bin/onnxruntime_providers_shared.dll,${CMAKE_BINARY_DIR}/vcpkg_installed/x64-windows/bin/onnxruntime_providers_shared.dll>"
                "$<TARGET_FILE_DIR:doodle_kitsu_supplement_test>"
)

add_test(NAME doodle_kitsu_supplement_test COMMAND doodle_kitsu_supplement_test --selftest)
