cmake_minimum_required(VERSION 3.8)
project(imgui-filebrowser CXX)

set(CMAKE_DEBUG_POSTFIX d)


add_library(${PROJECT_NAME} INTERFACE)

target_sources(${PROJECT_NAME} INTERFACE  ${PROJECT_SOURCE_DIR}/imfilebrowser.h)

install(FILES
        ${PROJECT_SOURCE_DIR}/imfilebrowser.h
        DESTINATION include
        )
