# CMakeList.txt: 顶层 CMake 项目文件，在此处执行全局配置
# 并包含子项目。
#
cmake_minimum_required (VERSION 3.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)
# 添加版本号
set(fileSyn_VERSION_MAJOR 0)
set(fileSyn_VERSION_MINOR 0.2)

configure_file(fileSynConfig.h.in fileSynConfig.h)

project ("doodle" VERSION 0.1.0)

# 包含子项目。
enable_testing()
add_subdirectory("filesyn")
add_subdirectory("googleTest")