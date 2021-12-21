# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindMaya
-------

Finds the Maya library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``Maya::Maya``
  The Maya library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``Maya_FOUND``
  True if the system has the Maya library.
``Maya_VERSION``
  The version of the Maya library which was found.
``Maya_INCLUDE_DIRS``
  Include directories needed to use Maya.
``Maya_LIBRARIES``
  Libraries needed to link to Maya.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``Maya_INCLUDE_DIR``
  The directory containing ``Maya.h``.
``Maya_LIBRARY``
  The path to the Maya library.

#]=======================================================================]


# 这个包设置${MAYA_INCLUDE_DIR}
#         ${MAYA_LIBRARY_DIR}
#         ${MAYA_LIBRARY}
#         ${MAYA_QT5_LIBRARY}
#这三个变量

include(CMakePrintHelpers)

if (WIN32)
    set(MAYA_DEFAULT_LOCATION "C:/Program Files/Autodesk/Maya${Maya_FIND_VERSION}")
    set(MAYA_FBX_LOCATION "C:/Program Files/Autodesk/FBX/FBX SDK/${Maya_FIND_VERSION}.0.1")
    set(OPEN_MAYA OpenMaya)
endif ()

unset(MAYA_BASE_DIR CACHE)
unset(MAYA_INCLUDE_DIR CACHE)
unset(MAYA_LIBRARY_DIR CACHE)

find_path(MAYA_BASE_DIR_${Maya_FIND_VERSION}
        include/maya/MFn.h
        HINTS
        ${MAYA_DEFAULT_LOCATION}
        DOC
        "maya 基本路径"
        )

find_path(MAYA_INCLUDE_DIR_${Maya_FIND_VERSION}
        maya/MFn.h
        HINTS
        ${MAYA_DEFAULT_LOCATION}
        PATH_SUFFIXES
        "include/"
        REQUIRED
        DOC
        "maya 导入路径"
        )

find_path(MAYA_LIBRARY_DIR_${Maya_FIND_VERSION}
        ${OPEN_MAYA}.lib
        HINTS
        ${MAYA_DEFAULT_LOCATION}
        PATH_SUFFIXES
        "lib"
        REQUIRED
        DOC
        "maya 连接库"
        )
find_path(MAYA_DLL_LIBRARY_DIR_${Maya_FIND_VERSION}
        ${OPEN_MAYA}.dll
        HINTS
        ${MAYA_DEFAULT_LOCATION}
        PATH_SUFFIXES
        "bin"
        REQUIRED
        DOC
        "maya dll 位置"
        )
#创建maya库列表
set(MAYA_LIBS_TO_FIND
        OpenMaya
        OpenMayaAnim
        OpenMayaFX
        OpenMayaRender
        OpenMayaUI
        Image
        Foundation
        MetaData # 元数据连接库
        # IMFbase
        cg
        cgGL
        clew
        )

add_library(maya_${Maya_FIND_VERSION}_all INTERFACE IMPORTED)
foreach (MAYA_LIB ${MAYA_LIBS_TO_FIND})
    find_library(MAYA_${MAYA_LIB}_LIBRARY_${Maya_FIND_VERSION}
            ${MAYA_LIB}
            HINTS
            ${MAYA_LIBRARY_DIR_${Maya_FIND_VERSION}}
            REQUIRED
            DOC
            "寻找maya ${MAYA_LIB}库"
            NO_CMAKE_SYSTEM_PATH
            )
    find_file(
            MAYA_${MAYA_LIB}_LIBRARY_dll_${Maya_FIND_VERSION}
            ${MAYA_LIB}.dll
            HINTS
            ${MAYA_DLL_LIBRARY_DIR_${Maya_FIND_VERSION}}
            REQUIRED
            DOC
            "寻找maya ${MAYA_LIB} dll库"
            NO_CMAKE_SYSTEM_PATH
    )

    if (MAYA_${MAYA_LIB}_LIBRARY_${Maya_FIND_VERSION})
        list(APPEND MAYA_LIBRARY ${MAYA_${MAYA_LIB}_LIBRARY_${Maya_FIND_VERSION}})
        # 添加基础库
        add_library(maya_${Maya_FIND_VERSION}_${MAYA_LIB} SHARED IMPORTED)
        # 添加基础库的别名
        add_library(maya::maya_${Maya_FIND_VERSION}_${MAYA_LIB} ALIAS maya_${Maya_FIND_VERSION}_${MAYA_LIB})
        # 设置属性
        set_property(TARGET maya_${Maya_FIND_VERSION}_${MAYA_LIB} APPEND PROPERTY
                IMPORTED_CONFIGURATIONS RELEASE)
        set_property(TARGET maya_${Maya_FIND_VERSION}_${MAYA_LIB} APPEND PROPERTY
                IMPORTED_CONFIGURATIONS DEBUG)
        target_include_directories(maya_${Maya_FIND_VERSION}_${MAYA_LIB} INTERFACE ${MAYA_INCLUDE_DIR_${Maya_FIND_VERSION}})

        set_target_properties(maya_${Maya_FIND_VERSION}_${MAYA_LIB} PROPERTIES
                IMPORTED_LOCATION "${MAYA_${MAYA_LIB}_LIBRARY_dll_${Maya_FIND_VERSION}}"
                IMPORTED_IMPLIB "${MAYA_${MAYA_LIB}_LIBRARY_${Maya_FIND_VERSION}}"
                )
        target_link_libraries(maya_${Maya_FIND_VERSION}_all INTERFACE maya_${Maya_FIND_VERSION}_${MAYA_LIB})
    endif ()
endforeach ()

#foreach(MAYA_QT5_LIB ${MAYA_LIBS_TO_QT_FIND})
#  find_library(MAYA_QT_${MAYA_QT5_LIB}_LIBRARY
#  ${MAYA_QT5_LIB}
#  HINTS
#   ${MAYA_LIBRARY_DIR}
#  REQUIRED
#  DOC
#   "寻找maya qt ${MAYA_QT5_LIB} 组件"
#  NO_CMAKE_PATH
#  NO_CMAKE_ENVIRONMENT_PATH
#  NO_CMAKE_SYSTEM_PATH
#  NO_DEFAULT_PATH
#  )
#  if(MAYA_QT_${MAYA_QT5_LIB}_LIBRARY)
#    list(APPEND MAYA_QT5_LIBRARY ${MAYA_QT_${MAYA_QT5_LIB}_LIBRARY})
#  endif()
#endforeach()


include(FindPackageHandleStandardArgs)


find_package_handle_standard_args(Maya
        FOUND_VAR
        Maya_FOUND
        REQUIRED_VARS
        MAYA_INCLUDE_DIR_${Maya_FIND_VERSION}
        MAYA_LIBRARY
        REASON_FAILURE_MESSAGE
        "maya没有找到"
        )
target_compile_definitions(
        maya_${Maya_FIND_VERSION}_all
        INTERFACE
        -DPARTIO_WIN32
        -DNT_PLUGIN
        -D_WIN32
        -DOSWin_
        -DUSERDLL
        -DCRT_SECURE_NO_DEPRECATE
        -D_HAS_ITERATOR_DEBUGGING=0
        -D_CRT_SECURE_NO_WARNINGS
)
#add_library(maya_${Maya_FIND_VERSION} SHARED IMPORTED)
#target_include_directories(maya_${Maya_FIND_VERSION} INTERFACE ${MAYA_INCLUDE_DIR})
##target_link_libraries(maya_${Maya_FIND_VERSION} INTERFACE ${MAYA_LIBRARY})
#
#set_property(TARGET maya_${Maya_FIND_VERSION} APPEND PROPERTY
#        IMPORTED_CONFIGURATIONS RELEASE)
#set_property(TARGET maya_${Maya_FIND_VERSION} APPEND PROPERTY
#        IMPORTED_CONFIGURATIONS DEBUG)
#
#set_target_properties(maya_${Maya_FIND_VERSION} PROPERTIES
#        IMPORTED_LOCATION_DEBUG "F:/src/Autodesk_Maya_2020_DEVKIT_Windows_Hotfix_1/devkitBase/lib/OpenMaya.lib"
#        IMPORTED_IMPLIB_DEBUG "F:/src/Autodesk_Maya_2020_DEVKIT_Windows_Hotfix_1/devkitBase/lib/OpenMaya.lib"
#        )
#set_target_properties(maya_${Maya_FIND_VERSION} PROPERTIES
#        IMPORTED_LOCATION_RELEASE "F:/src/Autodesk_Maya_2020_DEVKIT_Windows_Hotfix_1/devkitBase/lib/OpenMaya.lib"
#        IMPORTED_IMPLIB_RELEASE "F:/src/Autodesk_Maya_2020_DEVKIT_Windows_Hotfix_1/devkitBase/lib/OpenMaya.lib"
#        )
add_library(maya::maya_${Maya_FIND_VERSION}_all ALIAS maya_${Maya_FIND_VERSION}_all)
