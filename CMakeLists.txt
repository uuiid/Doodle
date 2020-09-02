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
configure_file(DoodleConfig.h.in ${CMAKE_CURRENT_LIST_DIR}/DoodleConfig.h)
find_package(Qt5 COMPONENTS Core Widgets Test LinguistTools REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem date_time)
find_package(CURL CONFIG REQUIRED)
add_subdirectory("ftp")
add_subdirectory("fileSyn")
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
install( TARGETS Doodle DESTINATION bin)
#install( FILES "")
enable_testing()
add_subdirectory("test")

