set(FIND_PACKAGE_INTERNAL_${CMAKE_FIND_PACKAGE_NAME} ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} CACHE STRING "内部缓存版本" FORCE)

if (WIN32)
    set(MAYA_DEFAULT_LOCATION "C:/Program Files/Autodesk/Maya${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION}")
    set(ADSKXGEN_LIB AdskXGen.lib)
elseif (APPLE)
    set(MAYA_DEFAULT_LOCATION "/Applications/Autodesk/maya${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION}")
    set(ADSKXGEN_LIB libAdskXGen.dylib)
elseif (LINUX)
    set(MAYA_DEFAULT_LOCATION "/usr/autodesk/maya${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION}")
    set(ADSKXGEN_LIB libAdskXGen.so)
endif ()

set(XGEN_PLUGIN plug-ins/xgen)
find_path(XGEN_LIB_DIR ${ADSKXGEN_LIB}
        HINTS
        ${MAYA_LOCATION}/${XGEN_PLUGIN}
        $ENV{MAYA_LOCATION}/${XGEN_PLUGIN}
        ${MAYA_DEFAULT_LOCATION}/${XGEN_PLUGIN}
        "[HKLM\\SOFTWARE\\Autodesk\\Maya\\${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION}\\Setup\\InstallPath;MAYA_INSTALL_LOCATION]"
        PATH_SUFFIXES
        "lib/"
        DOC
        "XGen library path"
)
cmake_path(GET XGEN_LIB_DIR PARENT_PATH XGEN_ROOT)

find_path(XGEN_INCLUDE_DIR
        xgen/src/xgcore/XgConfig.h
        HINTS
        ${MAYA_LOCATION}/${XGEN_PLUGIN}
        $ENV{MAYA_LOCATION}/${XGEN_PLUGIN}
        ${MAYA_DEFAULT_LOCATION}/${XGEN_PLUGIN}
        PATH_SUFFIXES
        "include/"
        DOC
        "XGen include path"
)

if (XGEN_LIB_DIR AND XGEN_INCLUDE_DIR)
    set(XGEN_FOUND TRUE)
endif ()
find_package_handle_standard_args(${CMAKE_FIND_PACKAGE_NAME}
        REQUIRED_VARS
        XGEN_LIB_DIR
        XGEN_INCLUDE_DIR
        REASON_FAILURE_MESSAGE "maya xgen 库中的组件没有找到"
)

add_library(xgen SHARED IMPORTED)
set_property(TARGET xgen APPEND PROPERTY
        IMPORTED_CONFIGURATIONS RELEASE)
set_property(TARGET xgen APPEND PROPERTY
        IMPORTED_CONFIGURATIONS DEBUG)
target_include_directories(xgen INTERFACE ${XGEN_INCLUDE_DIR})
set_target_properties(xgen PROPERTIES
        IMPORTED_LOCATION "${XGEN_ROOT}/bin/AdskXGen.dll"
        IMPORTED_IMPLIB "${XGEN_LIB_DIR}/${ADSKXGEN_LIB}"
)
add_library(maya::xgen ALIAS xgen)