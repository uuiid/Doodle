cmake_minimum_required(VERSION 3.8)
project(imguiwrap CXX)

set(CMAKE_DEBUG_POSTFIX d)

find_package(imgui)

add_library(${PROJECT_NAME} "")
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(
        ${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:src>
)
target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui)

target_sources(${PROJECT_NAME}
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.cpp
        )


install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_target
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)
install(FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.dear.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.defermacro.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.helpers.h
        DESTINATION include
        )
include(CMakePackageConfigHelpers)
configure_package_config_file(imgui-config.cmake.in imgui-config.cmake INSTALL_DESTINATION share/imgui)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/imguiwrap-config.cmake DESTINATION share/imguiwrap)

install(
        EXPORT ${PROJECT_NAME}_target
        NAMESPACE ${PROJECT_NAME}::
        FILE ${PROJECT_NAME}-targets.cmake
        DESTINATION share/${PROJECT_NAME}
)
