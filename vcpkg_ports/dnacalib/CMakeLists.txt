cmake_minimum_required(VERSION 3.24)
project(dnacalib VERSION 6.8.0 LANGUAGES CXX)

file(GLOB_RECURSE NDA_SOURCES
        RELATIVE ${PROJECT_SOURCE_DIR}
        "${PROJECT_SOURCE_DIR}/Engine/Plugins/Animation/DNACalib/Source/DNACalibLib/Private/*.cpp"
)
file(GLOB_RECURSE NDA_HEADERS
        RELATIVE ${PROJECT_SOURCE_DIR}
        "${PROJECT_SOURCE_DIR}/Engine/Plugins/Animation/DNACalib/Source/DNACalibLib/Public/*.h"
)

file(GLOB_RECURSE RIG_SOURCES
        RELATIVE ${PROJECT_SOURCE_DIR}
        "${PROJECT_SOURCE_DIR}/Engine/Plugins/Animation/RigLogic/Source/RigLogicLib/Private/*.cpp"
)
file(GLOB_RECURSE RIG_HEADERS
        RELATIVE ${PROJECT_SOURCE_DIR}
        "${PROJECT_SOURCE_DIR}/Engine/Plugins/Animation/RigLogic/Source/RigLogicLib/Public/*.h"
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(DNAC_LIBRARY_TYPE SHARED)

add_library(${PROJECT_NAME} ${DNAC_LIBRARY_TYPE})
add_library(DNACalib::dnacalib ALIAS ${PROJECT_NAME})

set_target_properties(${PROJECT_NAME} PROPERTIES
        CXX_EXTENSIONS NO
        CXX_VISIBILITY_PRESET hidden
        SOVERSION ${PROJECT_VERSION_MAJOR}
        VERSION ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH})

if (DNAC_LIBRARY_TYPE STREQUAL SHARED)
    target_compile_definitions(${PROJECT_NAME}
            PUBLIC
            RL_SHARED
            DNAC_SHARED
            RIGLOGIC_MODULE_DISCARD
            DNACALIB_MODULE_DISCARD

            PRIVATE
            DNAC_BUILD_SHARED
            RL_BUILD_SHARED
            RL_BUILD_WITH_ZYX_ROTATION_ORDER
            TRIO_WINDOWS_FILE_MAPPING_AVAILABLE
            RL_AUTODETECT_SSE
            RL_AUTODETECT_HALF_FLOATS
            # UE宏
            #            TRIMD_CUSTOM_WINDOWS_H="WindowsPlatformUE.h"
            #            TRIO_CUSTOM_WINDOWS_H="WindowsPlatformUE.h"
            #            UE_BUILD_SHIPPING
            #            WITH_EDITOR=0
            #            WITH_ENGINE=0
            #            WITH_UNREAL_DEVELOPER_TOOLS=0
            #            WITH_PLUGIN_SUPPORT=0
            #            IS_MONOLITHIC=0
            #            IS_PROGRAM
            #            OVERRIDE_PLATFORM_HEADER_NAME=Windows
            #            WINVER=0x0602
            #            PLATFORM_WINDOWS
    )
endif ()

message("CMAKE_INSTALL_INCLUDEDIR = ${CMAKE_INSTALL_INCLUDEDIR}")
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Engine/Plugins/Animation/DNACalib/Source/DNACalibLib/Public>
        #        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/Engine/Plugins/Animation/RigLogic/Source/RigLogicLib/Public>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Plugins/Animation/DNACalib/Source/DNACalibLib/Private
        ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Plugins/Animation/RigLogic/Source/RigLogicLib/Private
        # ${CMAKE_CURRENT_SOURCE_DIR}/Engine/Source/Runtime/Core/Public
)

target_sources(
        ${PROJECT_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${NDA_SOURCES}>
        $<BUILD_INTERFACE:${RIG_SOURCES}>
        PUBLIC
        FILE_SET headers1
        TYPE HEADERS
        BASE_DIRS Engine/Plugins/Animation/DNACalib/Source/DNACalibLib/Public
        FILES ${NDA_HEADERS}
        FILE_SET headers2
        TYPE HEADERS
        BASE_DIRS Engine/Plugins/Animation/RigLogic/Source/RigLogicLib/Public
        FILES ${RIG_HEADERS}
)

set(COMPONENT_NAME ${target_name})
set(INSTALL_CONFIGDIR share/${PROJECT_NAME})

# Set install destinations and associate installed target files with an export
install(TARGETS ${PROJECT_NAME}
        EXPORT ${PROJECT_NAME}-targets
        FILE_SET headers1 DESTINATION include
        FILE_SET headers2 DESTINATION include
)

# Write build-tree targets
export(TARGETS ${PROJECT_NAME}
        FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::)
# Allow find_package to locate package without installing it (find it's build-tree)
export(PACKAGE ${PROJECT_NAME})

# Write install-tree targets
install(EXPORT ${PROJECT_NAME}-targets
        FILE ${PROJECT_NAME}Targets.cmake
        NAMESPACE ${PROJECT_NAME}::
        DESTINATION ${INSTALL_CONFIGDIR}
)

include(CMakePackageConfigHelpers)

# Generate build-tree configuration
#set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
#set(LIB_DIR ${CMAKE_CURRENT_BINARY_DIR})
#configure_package_config_file("${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in"
#        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
#        INSTALL_DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
#        PATH_VARS INCLUDE_DIR LIB_DIR
#        INSTALL_PREFIX ${CMAKE_CURRENT_BINARY_DIR})

# Generate install-tree configuration
set(INCLUDE_DIR include)
set(LIB_DIR lib)
configure_package_config_file("${CMAKE_CURRENT_LIST_DIR}/Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.install.cmake"
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR}
        PATH_VARS INCLUDE_DIR LIB_DIR)

# Generate package version file
write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)

# Install the install-tree configuration
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.install.cmake"
        DESTINATION ${INSTALL_CONFIGDIR}
        RENAME "${PROJECT_NAME}Config.cmake"
        COMPONENT ${COMPONENT_NAME})

# Install package version file
install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
        DESTINATION ${INSTALL_CONFIGDIR}
        COMPONENT ${COMPONENT_NAME})

# Install include files
#install(DIRECTORY include/
#        DESTINATION include
#        COMPONENT ${COMPONENT_NAME})