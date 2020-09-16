cmake_minimum_required(VERSION 3.5)

project(Doodle LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# QtCreator supports the following variables for Android, which are identical to qmake Android variables.
# Check http://doc.qt.io/qt-5/deployment-android.html for more information.
# They need to be set before the find_package(Qt5 ...) call.

#if(ANDROID)
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
#    if (ANDROID_ABI STREQUAL "armeabi-v7a")
#        set(ANDROID_EXTRA_LIBS
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libcrypto.so
#            ${CMAKE_CURRENT_SOURCE_DIR}/path/to/libssl.so)
#    endif()
#endif()
set(CMAKE_INSTALL_PREFIX "F:/Source/qt_test/install")
configure_file(DoodleConfig.h.in ${CMAKE_CURRENT_LIST_DIR}/DoodleConfig.h)
find_package(Qt5 COMPONENTS Core Widgets Test LinguistTools REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem date_time)
find_package(CURL REQUIRED)
#include_directories(${CURL_INCLUDE_DIRS})
#message(${CURL_INCLUDE_DIRS})
# 添加orm
#add_subdirectory(QxOrm ./QxOrm)
#include(QxOrm/QxOrm.cmake)

add_subdirectory(ftp)
add_subdirectory(fileSyn)
add_subdirectory(pinyin)
add_subdirectory(core)

set(TS_FILES test_cmake_zh_CN.ts)

if(ANDROID)
  add_library(Doodle SHARED
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ${TS_FILES}
  )
else()
  add_executable(Doodle
    main.cpp
    mainwindow.cpp
    mainwindow.h
    mainwindow.ui
    ${TS_FILES}
  )
endif()

target_link_libraries(Doodle PRIVATE Qt5::Widgets fileSyn)

qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
install( TARGETS Doodle
    RUNTIME DESTINATION Doodle
    BUNDLE  DESTINATION Doodle)
#启用测试
enable_testing()
add_subdirectory("test")

#=====================安装打包过程===============================
#首先搜索我们的自定义函数
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
include(mypack)
#搜索qt 安装辅助程序
find_program(WINQT windeployqt REQUIRED)
#添加qt 安装辅助程序的一些命令
add_custom_command(
    TARGET Doodle POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E rm -r -f ${CMAKE_INSTALL_PREFIX}/Doodle
    COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/Doodle $<TARGET_FILE:Doodle>
    )
install(
    DIRECTORY ${CMAKE_INSTALL_PREFIX}/Doodle/
    DESTINATION ${PROJECT_NAME}
    )
CPACKIFW_COMMON()
#显示所有变量
#dump_cmake_variables()

#====================一些非qt组件的搜索和安装=============
#第一种搜索方式
include(InstallRequiredSystemLibraries)
#第二中验证加搜索方式
set(APPS ${CMAKE_INSTALL_PREFIX}/Doodle/Doodle.exe)  # paths to executables
get_filename_component(fixdir ${QT_QMAKE_EXECUTABLE} DIRECTORY)
set(DIRS ${CMAKE_FIND_ROOT_PATH}
    ${fixdir}
    ${CMAKE_INSTALL_PREFIX}/lib)

install(CODE "
    include(BundleUtilities)
    fixup_bundle(\"${APPS}\"   \"\"   \"${DIRS}\")
    ")

include(CPack)
