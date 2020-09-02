#cmake_minimum_required(VERSION 3.5)

#project(fileSyn LANGUAGES CXX)

#set(CMAKE_INCLUDE_CURRENT_DIR ON)
#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)
#set(CMAKE_CXX_STANDARD 14)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_PREFIX_PATH "F:\\Qt\\5.12.9\\msvc2017_64")
find_package(Qt5 COMPONENTS Core Sql LinguistTools REQUIRED)
#include_directories(${Qt5Core_INCLUDE_DIRS})


set(TS_FILES fileSyn_zh_CN.ts)
set(HEADER_FILES
    fileSyn_global.h
    src/filesyn.h
    src/sqlconnect.h
    src/foldercompare.h
    src/fileinfo.h
    src/d_setting.h)

add_library(fileSyn SHARED
  ${HEADER_FILES}
  src/filesyn.cpp
  src/sqlconnect.cpp
  src/foldercompare.cpp
  src/fileinfo.cpp
  src/d_setting.cpp
  ${TS_FILES}
)

target_link_libraries(fileSyn PRIVATE Qt5::Sql Qt5::Core )

target_compile_definitions(fileSyn PRIVATE FILESYN_LIBRARY)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})

target_include_directories(fileSyn PUBLIC ${CMAKE_CURRENT_LIST_DIR})

install(TARGETS fileSyn DESTINATION lib)
install(FILES ${HEADER_FILES} DESTINATION include)
