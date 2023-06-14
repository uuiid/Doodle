cmake_minimum_required(VERSION 3.15)

project(dem-bones
        VERSION 1.2.1)

add_library(${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:include/DemBones/DemBonesExt.h>
        $<BUILD_INTERFACE:include/DemBones/DemBones.h>
        $<BUILD_INTERFACE:include/DemBones/Indexing.h>
        $<BUILD_INTERFACE:include/DemBones/MatBlocks.h>
        $<BUILD_INTERFACE:include/DemBones/ConvexLS.h>
        )
include(GNUInstallDirs)
target_include_directories(
        ${PROJECT_NAME} INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
include(CMakePackageConfigHelpers)

install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}Targets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )

configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/cmake
        )

write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}"
        COMPATIBILITY AnyNewerVersion
)
install(EXPORT ${PROJECT_NAME}Targets
        FILE ${PROJECT_NAME}Targets.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/cmake
        NAMESPACE DemBones::
        )
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/${PROJECT_NAME}/cmake
        )

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# export(EXPORT ${PROJECT_NAME}Targets
# FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake"
# )
export(PACKAGE ${PROJECT_NAME})
