# CMakeList.txt: filesyn 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)
# 
#set(cmake_pix)

find_package(SQLite3 REQUIRED)
find_package(Boost REQUIRED)

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${PROJECT_BINARY_DIR})
message(${PROJECT_SOURCE_DIR})
# 将源代码添加到此项目的可执行文件。
add_executable (filesyn "filesyn.cpp" "filesyn.h" "fileInfo.cpp" "fileInfo.h")

# TODO: 如有需要，请添加测试并安装目标。
