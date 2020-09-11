find_package(Qt5 COMPONENTS Core Test REQUIRED)

add_executable(test_fileSyn
    "t_fileSyn/test_filesyn.h"
    "t_fileSyn/test_filesyn.cpp"
    "t_ftp/test_ftp.h"
    "t_ftp/test_ftp.cpp"
    "t_convert/test_convert.h"
    "t_convert/test_convert.cpp"
    "t_core/test_core.h"
    "t_core/test_core.cpp"
    "main.cpp"
    )

target_link_libraries(test_fileSyn
    PRIVATE Qt5::Test fileSyn ftp pinyin core)
add_test(NAME AllTestsInMain COMMAND test_fileSyn)
