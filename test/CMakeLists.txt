#set(CMAKE_INCLUDE_CURRENT_DIR ON)

#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)

#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt5 COMPONENTS Core Test REQUIRED)
#add_subdirectory("fileSyn")
#include_directories(${PROJECT_SOURCE_DIR})
#include_directories(${Qt5Test_INCLUDE_DIRS})
add_executable(test_fileSyn
    "t_fileSyn/test_filesyn.h"
    "t_fileSyn/test_filesyn.cpp"
    "t_fileSyn/main.cpp"
    )

target_link_libraries(test_fileSyn
    PRIVATE Qt5::Test fileSyn)
add_test(NAME AllTestsInMain COMMAND test_fileSyn)
