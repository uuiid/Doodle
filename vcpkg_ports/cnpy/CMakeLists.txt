cmake_minimum_required(VERSION 3.0 FATAL_ERROR)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
endif()

project(cnpy VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)

option(ENABLE_STATIC "Build static library" ON)
option(ENABLE_SHARED "Build shared library" ON)

find_package(ZLIB REQUIRED)

if(ENABLE_SHARED)
    add_library(cnpy SHARED "cnpy.cpp")
    target_include_directories(cnpy PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(cnpy PUBLIC ZLIB::ZLIB)
    set_target_properties(cnpy PROPERTIES
        PUBLIC_HEADER "cnpy.h"
        VERSION ${PROJECT_VERSION}
        SOVERSION 1
    )
    install(TARGETS cnpy
        EXPORT cnpyTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

if(ENABLE_STATIC)
    add_library(cnpy_static STATIC "cnpy.cpp")
    target_include_directories(cnpy_static PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)
    target_link_libraries(cnpy_static PUBLIC ZLIB::ZLIB)
    set_target_properties(cnpy_static PROPERTIES
        OUTPUT_NAME "cnpy"
        PUBLIC_HEADER "cnpy.h"
    )
    install(TARGETS cnpy_static
        EXPORT cnpyTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    )
endif()

# Install header
install(FILES "cnpy.h" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# CMake config
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cnpyConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cnpy
)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/cnpyConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion
)
install(EXPORT cnpyTargets
    FILE cnpyTargets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cnpy
    NAMESPACE cnpy::
)
install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cnpyConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cnpyConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cnpy
)