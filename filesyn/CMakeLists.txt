# CMakeList.txt: filesyn 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
# cmake_minimum_required (VERSION 3.8)
# 
#set(cmake_pix)

find_package(SQLite3 CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem date_time )

include_directories(${Boost_INCLUDE_DIRS})
include_directories(${SQLite3_INCLUDE_DIRS})
# message(" ++++++++++++++++++${SQLite3_INCLUDE_DIRS}")
include_directories(${PROJECT_BINARY_DIR})
# message(${PROJECT_SOURCE_DIR})
# 将源代码添加到此项目的可执行文件

get_cmake_property(_variableNames VARIABLES)
foreach (_variableName ${_variableNames})
    message(STATUS "${_variableName}=${${_variableName}}")
endforeach()

add_library(filesyn STATIC 
"filesyn.cpp"
"filesyn.h" 
"fileInfo.cpp" 
"fileInfo.h" 
"setting.cpp" 
"setting.h" "sqlConnect.h" "sqlConnect.cpp" "folderCompare.h" "folderCompare.cpp")
#target_link_directories(filesyn Boost::filesystem ${SQLite3_LIBRARY})
message("================ " )
target_link_libraries(filesyn Boost::filesystem sqlite3 )
# target_compile_definitions(filesyn INTERFACE USING_filesyn_LIB)
# install(TARGETS filesyn "${PROJECT_BINARY_DIR}/out")