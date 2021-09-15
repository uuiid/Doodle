cmake_minimum_required(VERSION 3.8)
project(imguiwrap CXX)

set(CMAKE_DEBUG_POSTFIX d)

option(IMGUIWRAP_BUILD_CORE "only header not glfw-backed main loop" ON)

find_package(imgui)

if (IMGUIWRAP_BUILD_CORE)
    add_library(${PROJECT_NAME} INTERFACE)
    target_include_directories(
            ${PROJECT_NAME}
            INTERFACE
            $<INSTALL_INTERFACE:src>
    )
    target_link_libraries(${PROJECT_NAME} INTERFACE imgui::imgui)
else ()
    add_library(${PROJECT_NAME} "")
    target_include_directories(
            ${PROJECT_NAME}
            PUBLIC
            $<INSTALL_INTERFACE:src>
    )
    target_link_libraries(${PROJECT_NAME} PUBLIC imgui::imgui)
endif ()
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})



if (NOT IMGUIWRAP_BUILD_CORE)
    target_sources(${PROJECT_NAME}
            PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.cpp
            )

endif ()


install(
        TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}_target
        ARCHIVE DESTINATION lib
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
)

if (IMGUIWRAP_BUILD_CORE)
    install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.dear.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.helpers.h
            DESTINATION include
            )
else ()
    install(FILES
            ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.dear.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.defermacro.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.h
            ${CMAKE_CURRENT_SOURCE_DIR}/src/imguiwrap.helpers.h
            DESTINATION include
            )
endif ()

include(CMakePackageConfigHelpers)
configure_package_config_file(imguiwrap-config.cmake.in imguiwrap-config.cmake INSTALL_DESTINATION share/imgui)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/imguiwrap-config.cmake DESTINATION share/imguiwrap)

install(
        EXPORT ${PROJECT_NAME}_target
        NAMESPACE ${PROJECT_NAME}::
        FILE ${PROJECT_NAME}-targets.cmake
        DESTINATION share/${PROJECT_NAME}
)
