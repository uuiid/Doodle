add_executable(test_my
        main.cpp
        test_convert.cpp
        test_core.cpp
        test_boost.cpp 
        test_qtCore.cpp
        test_std.cpp
        test_opencv.cpp
        doodle_test.manifest
        )

set_target_properties(test_my 
PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

#连接自己的库进行测试
target_link_libraries(test_my
        PRIVATE core pinyin logger)

find_library(DOODLE_MAR 
mpr.lib
REQUIRED
) 

target_link_libraries(test_my PRIVATE ${DOODLE_MAR})

# target_link_libraries(test_my PRIVATE cppzmq cppzmq-static)
target_link_libraries(test_my PRIVATE Qt5::Core)
target_compile_definitions(test_my PRIVATE _WIN32_WINNT=0x0A00)
#连接谷歌测试框架
target_link_libraries(test_my
        PRIVATE GTest::gtest GTest::gtest_main GTest::gmock GTest::gmock_main)

target_link_libraries(test_my PRIVATE
 Boost::locale
 ${OpenCV_LIBS}
 Qt5::Gui
 Qt5::Widgets
 )

# include(CTest)
enable_testing()
include(GoogleTest)
# gtest_discover_tests(test_my
# WORKING_DIRECTORY f:/Source/qt_test/Doodle/install/bin
# )

#安装基本组件
install(TARGETS test_my
        DESTINATION bin
        CONFIGURATIONS Debug)

#安装调试文件
install(FILES $<TARGET_PDB_FILE:test_my>
        DESTINATION bin
        CONFIGURATIONS Debug)

