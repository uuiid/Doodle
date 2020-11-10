cmake_minimum_required(VERSION 3.14)

project(Doodle LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#使用中文
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")


#启用测试
set(CMAKE_INSTALL_PREFIX "F:/Source/qt_test/Doodle/install")
configure_file(DoodleConfig.h.in ${CMAKE_CURRENT_LIST_DIR}/DoodleConfig.h)

list(APPEND CMAKE_PREFIX_PATH $ENV{QT_MYROOT}/5.12.9/msvc2017_64)

find_package(Qt5 CONFIG COMPONENTS Core Widgets Test LinguistTools REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem date_time)
find_package(CURL REQUIRED)
#include_directories(${CURL_INCLUDE_DIRS})
#message(${CURL_INCLUDE_DIRS})
# 添加orm
#add_subdirectory(QxOrm ./QxOrm)
#include(QxOrm/QxOrm.cmake)

add_subdirectory(logger)
add_subdirectory(ftp)
add_subdirectory(fileSyn)
add_subdirectory(pinyin)
add_subdirectory(core)
#add_subdirectory(doodle_GUI)
# set(TS_FILES test_cmake_zh_CN.ts)

# if(ANDROID)
# add_library(Doodle SHARED
# main.cpp
# mainwindow.cpp
# mainwindow.h
# mainwindow.ui
# ${TS_FILES}
# )
# else()
# add_executable(Doodle
# main.cpp
# mainwindow.cpp
# mainwindow.h
# mainwindow.ui
# ${TS_FILES}
# )
# endif()

# target_link_libraries(Doodle PRIVATE Qt5::Widgets)

# qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
# install( TARGETS Doodle
# RUNTIME DESTINATION Doodle
# BUNDLE  DESTINATION Doodle)

#添加测试
find_package(GTest CONFIG REQUIRED)

#enable_testing()
add_subdirectory(test)
#=====================安装打包过程===============================
#首先搜索我们的自定义函数
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
include(mypack)
#搜索qt 安装辅助程序
find_program(WINQT windeployqt REQUIRED)
#添加qt 安装辅助程序的一些命令


#install(
#        DIRECTORY ${CMAKE_INSTALL_PREFIX}/bin/
#        DESTINATION ${PROJECT_NAME}
#)
#CPACKIFW_COMMON()
#显示所有变量
#dump_cmake_variables()

#====================一些非qt组件的搜索和安装=============
#第一种搜索方式
include(InstallRequiredSystemLibraries)
#第二中验证加搜索方式
set(DOODLEAPPS ${CMAKE_INSTALL_PREFIX}/bin/doodle.exe)  # paths to executables
set(TESTAPPS ${CMAKE_INSTALL_PREFIX}/bin/test_my.exe)
get_filename_component(fixdir ${WINQT} DIRECTORY)

list(APPEND DIRS ${fixdir})
list(APPEND DIRS ${CMAKE_INSTALL_PREFIX}/lib)
foreach(_var in LISTS ${CMAKE_FIND_ROOT_PATH})
     list(APPEND DIRS ${_var}/bin)
endforeach(_var)
list(APPEND DIRS "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/")
list(APPEND DIRS "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/Remote Debugger/x64/")

#set(DIRS ${CMAKE_FIND_ROOT_PATH}/bin
#        ${fixdir}
#        ${CMAKE_INSTALL_PREFIX}/lib)
#复制tool
install(DIRECTORY ${CMAKE_SOURCE_DIR}/tools/
        DESTINATION tools)

#最后实现camke修复
install(CODE "
     include(BundleUtilities)
     fixup_bundle(\"${DOODLEAPPS}\"   \"\"   \"${DIRS}\")
     ")
install(CODE "
     include(BundleUtilities)
     fixup_bundle(\"${TESTAPPS}\"   \"\"   \"${DIRS}\")
     ")
# include(CPack)
# ${projectDir}\out\install\${name}