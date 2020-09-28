
    # test_filesyn.cpp
    # test_ftp.cpp
    # test_convert.cpp
add_executable(test_my
    test_core.cpp
    main.cpp
    )
#连接自己的库进行测试
target_link_libraries(test_my
    PRIVATE core fileSyn ftp pinyin )
#连接谷歌测试框架
target_link_libraries(test_my
    PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)
# include(CTest)
enable_testing()
include(GoogleTest)
# add_test(alltest test_my)
# gtest_discover_tests(test_my
#     PROPERTIES
#     ENVIRONMENT "PATH=$ENV{QT_MYROOT}/5.12.9/msvc2017_64;$ENV{QT_MYROOT}/5.12.9/msvc2017_64/bin;$ENV{QT_MYROOT}/5.12.9/msvc2017_64/plugins;${CMAKE_BUILD_DIR}/build/core/Debug/;${CMAKE_BUILD_DIR}/build/fileSyn/Debug/;${CMAKE_BUILD_DIR}/build/ftp/Debug/;${CMAKE_BUILD_DIR}/build/pinyin/Debug/;${CMAKE_BUILD_DIR}/build/;$ENV{VCPKG_ROOT}/vcpkg/installed/x64-windows/bin;$ENV{VCPKG_ROOT}/vcpkg/installed/x64-windows/debug/bin;C:/Program Files/MySQL/MySQL Server 8.0/lib\;$ENV{PATH}"
# )

# set_tests_properties(alltest
# PROPERTIES
#     ENVIRONMENT "PATH=$ENV{QT_MYROOT}/5.12.9/msvc2017_64;$ENV{QT_MYROOT}/5.12.9/msvc2017_64/bin;$ENV{QT_MYROOT}/5.12.9/msvc2017_64/plugins;${CMAKE_BUILD_DIR}/build/core/Debug/;${CMAKE_BUILD_DIR}/build/fileSyn/Debug/;${CMAKE_BUILD_DIR}/build/ftp/Debug/;${CMAKE_BUILD_DIR}/build/pinyin/Debug/;${CMAKE_BUILD_DIR}/build/;$ENV{VCPKG_ROOT}/vcpkg/installed/x64-windows/bin;$ENV{VCPKG_ROOT}/vcpkg/installed/x64-windows/debug/bin;C:/Program Files/MySQL/MySQL Server 8.0/lib\;$ENV{PATH}"
#     )
# include(CMakePrintHelpers)
# cmake_print_properties(
#     TARGETS test_my
#     PROPERTIES ENVIRONMENT
# )
# set_tests_properties(
#     name_of_test_one
#     name_of_test_two
# PROPERTIES
#     # Make sure DLL is found by adding its directory to PATH
#     ENVIRONMENT "PATH=$<TARGET_FILE_DIR:library_target_name>\;$ENV{PATH}"
# )