



include(FindPackageHandleStandardArgs)

set(${CMAKE_FIND_PACKAGE_NAME}_SDK_ROOT "C:/Program Files/Autodesk/FBX/FBX SDK")

if (WIN32)
    set(${CMAKE_FIND_PACKAGE_NAME}_SDK_ROOT "${${CMAKE_FIND_PACKAGE_NAME}_SDK_ROOT}/${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION_MAJOR}.${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION_MINOR}.${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION_PATCH}" "")
endif ()


find_path(${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR "fbxsdk.h"
        PATHS ${${CMAKE_FIND_PACKAGE_NAME}_SDK_ROOT}
        PATH_SUFFIXES "include"
)

find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY_DEBUG "libfbxsdk.lib"
        PATHS ${${CMAKE_FIND_PACKAGE_NAME}_SDK_ROOT}
        PATH_SUFFIXES "lib/vs2022/x64/debug"
)
find_library(${CMAKE_FIND_PACKAGE_NAME}_DLL_DEBUG "libfbxsdk.dll"
        PATHS ${${CMAKE_FIND_PACKAGE_NAME}_SDK_ROOT}
        PATH_SUFFIXES "lib/vs2022/x64/debug"
)

find_library(${CMAKE_FIND_PACKAGE_NAME}_LIBRARY_RELEASE "libfbxsdk.lib"
        PATHS ${${CMAKE_FIND_PACKAGE_NAME}_SDK_ROOT}
        PATH_SUFFIXES "lib/vs2022/x64/release"
)
find_library(${CMAKE_FIND_PACKAGE_NAME}_DLL_RELEASE "libfbxsdk.dll"
        PATHS ${${CMAKE_FIND_PACKAGE_NAME}_SDK_ROOT}
        PATH_SUFFIXES "lib/vs2022/x64/release"
)

add_library(${CMAKE_FIND_PACKAGE_NAME} SHARED IMPORTED)
target_include_directories(${CMAKE_FIND_PACKAGE_NAME} INTERFACE ${${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR})
set_target_properties(${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
        IMPORTED_IMPLIB_DEBUG "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY_DEBUG}"
        IMPORTED_LOCATION_DEBUG "${${CMAKE_FIND_PACKAGE_NAME}_DLL_DEBUG}"
        IMPORTED_IMPLIB_RELEASE "${${CMAKE_FIND_PACKAGE_NAME}_LIBRARY_RELEASE}"
        IMPORTED_LOCATION_RELEASE "${${CMAKE_FIND_PACKAGE_NAME}_DLL_RELEASE}"
)
set_property(TARGET ${CMAKE_FIND_PACKAGE_NAME}
        APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG
)
set_property(TARGET ${CMAKE_FIND_PACKAGE_NAME}
        APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE
)
find_package_handle_standard_args(
        ${CMAKE_FIND_PACKAGE_NAME}
        REQUIRED_VARS ${CMAKE_FIND_PACKAGE_NAME}_INCLUDE_DIR ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY_DEBUG ${CMAKE_FIND_PACKAGE_NAME}_LIBRARY_RELEASE
        REASON_FAILURE_MESSAGE "Could NOT find ${CMAKE_FIND_PACKAGE_NAME}"
)
add_library(${CMAKE_FIND_PACKAGE_NAME}::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${CMAKE_FIND_PACKAGE_NAME})