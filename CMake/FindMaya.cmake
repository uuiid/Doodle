# Distributed under the GNU 3.0 License.  See accompanying

#[=======================================================================[.rst:
FindMaya
-------

Finds the Maya library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``maya::maya_${Maya_VERSION}_all``
  The Maya library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:


``Maya_INCLUDE_DIRS``
  Include directories needed to use Maya.
``Maya_LIBRARIES``
  Libraries needed to link to Maya.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``MAYA_INCLUDE_DIR``
  The directory containing ``Maya.h``.
``MAYA_BASE_DIR``
  maya 基本的路径，在注册表中和 MAYA_LOCATION Maya_ROOT_DIR中寻找
`` maya_all ``
  The path to the Maya library.

#]=======================================================================]

include(CMakePrintHelpers)

set(OPEN_MAYA OpenMaya)
if (WIN32)
    # 现在的使用默认的maya安装路径寻找， 由于工作是在 win 平台上的， 并没有兼容其他的平台
endif ()

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

set(QT_LIBS_TO_FIND
        Qt5
        Qt5Core
        Qt5Gui
        Qt5Widgets
        )

if (NOT ${FIND_PACKAGE_INTERNAL_${CMAKE_FIND_PACKAGE_NAME}} EQUAL ${Maya_FIND_VERSION})
    message("clear old maya version ${FIND_PACKAGE_INTERNAL_${CMAKE_FIND_PACKAGE_NAME}} ")
    unset(MAYA_BASE_DIR CACHE)
    unset(MAYA_INCLUDE_DIR CACHE)
    unset(MAYA_LIBRARY_DIR CACHE)
    unset(MAYA_DLL_LIBRARY_DIR CACHE)

    foreach (MAYA_LIB ${MAYA_LIBS_TO_FIND})
        unset(MAYA_${MAYA_LIB}_LIBRARY CACHE)
        unset(MAYA_${MAYA_LIB}_LIBRARY_dll CACHE)
    endforeach ()

    foreach (QT_LIB ${QT_LIBS_TO_FIND})
        unset(${QT_LIB}_DIR CACHE)
    endforeach ()

endif ()


set(FIND_PACKAGE_INTERNAL_${CMAKE_FIND_PACKAGE_NAME} ${Maya_FIND_VERSION} CACHE STRING "内部缓存版本" FORCE)


# 寻找maya 中的基本路径 使用 添加版本
# MAYA_DEFAULT_LOCATION -> C:/Program Files/Autodesk/Maya2018
# 验证 C:/Program Files/Autodesk/Maya2018/include/maya/MFn.h 是否存在 -> 存在 -> 设置变量 MAYA_BASE_DIR 为 C:/Program Files/Autodesk/Maya2018
# 验证 D:/Maya2018/include/maya/MFn.h 是否存在 -> 存在 -> 设置变量 MAYA_BASE_DIR 为 D:/Maya2018
find_path(MAYA_BASE_DIR
        include/maya/MFn.h
        HINTS
        "${MAYA_LOCATION}"
        "$ENV{MAYA_LOCATION}"
        "${Maya_ROOT_DIR}"
        "[HKLM\\SOFTWARE\\Autodesk\\Maya\\${Maya_FIND_VERSION}\\Setup\\InstallPath;MAYA_INSTALL_LOCATION]"
        DOC
        "maya 基本路径"
        )

# 寻找maya 中的基本导入路径 使用 添加版本
find_path(MAYA_INCLUDE_DIR
        maya/MFn.h
        HINTS
        "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
        "include"
        DOC
        "maya 导入路径"
        )

# 寻找maya 中的库路径 使用 添加版本
find_path(MAYA_LIBRARY_DIR
        ${OPEN_MAYA}.lib
        HINTS
        "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
        "lib"
        DOC
        "maya 连接库"
        )

# 寻找maya 中的动态库路径 使用 添加版本
find_path(MAYA_DLL_LIBRARY_DIR
        ${OPEN_MAYA}.dll
        HINTS
        "${MAYA_BASE_DIR}"
        PATH_SUFFIXES
        "bin"
        DOC
        "maya dll 位置"
        )

# 添加maya 接口库
add_library(maya_all INTERFACE IMPORTED)
target_include_directories(maya_all INTERFACE ${MAYA_INCLUDE_DIR})
# 循环查找maya 库列表
foreach (MAYA_LIB ${MAYA_LIBS_TO_FIND})
    find_library(MAYA_${MAYA_LIB}_LIBRARY
            ${MAYA_LIB}
            HINTS
            ${MAYA_LIBRARY_DIR}
            DOC
            "寻找maya ${MAYA_LIB}库"
            NO_CMAKE_SYSTEM_PATH
            )
    find_file(
            MAYA_${MAYA_LIB}_LIBRARY_dll
            ${MAYA_LIB}.dll
            HINTS
            ${MAYA_DLL_LIBRARY_DIR}
            DOC
            "寻找maya ${MAYA_LIB} dll库"
            NO_CMAKE_SYSTEM_PATH
    )

    if (MAYA_${MAYA_LIB}_LIBRARY)
        list(APPEND MAYA_LIBRARY ${MAYA_${MAYA_LIB}_LIBRARY})
        # 添加基础库
        add_library(maya_${MAYA_LIB} SHARED IMPORTED)
        # 添加基础库的别名
        add_library(maya::maya_${MAYA_LIB} ALIAS maya_${MAYA_LIB})
        # 设置属性
        set_property(TARGET maya_${MAYA_LIB} APPEND PROPERTY
                IMPORTED_CONFIGURATIONS RELEASE)
        set_property(TARGET maya_${MAYA_LIB} APPEND PROPERTY
                IMPORTED_CONFIGURATIONS DEBUG)
        target_include_directories(maya_${MAYA_LIB} INTERFACE ${MAYA_INCLUDE_DIR})

        set_target_properties(maya_${MAYA_LIB} PROPERTIES
                IMPORTED_LOCATION "${MAYA_${MAYA_LIB}_LIBRARY_dll}"
                IMPORTED_IMPLIB "${MAYA_${MAYA_LIB}_LIBRARY}"
                )
        target_link_libraries(maya_all INTERFACE maya_${MAYA_LIB})
    endif ()
endforeach ()

#foreach(MAYA_QT5_LIB ${MAYA_LIBS_TO_QT_FIND})
#  find_library(MAYA_QT_${MAYA_QT5_LIB}_LIBRARY
#  ${MAYA_QT5_LIB}
#  HINTS
#   ${MAYA_LIBRARY_DIR}
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
        REQUIRED_VARS
        MAYA_INCLUDE_DIR MAYA_LIBRARY_DIR
        REASON_FAILURE_MESSAGE "maya 库中的组件没有找到"
        )
if (Maya_FOUND)
    # 这些宏选项都在 maya devkit.cmake 文件中有, 需要复制过来
    target_compile_definitions(
            maya_all
            INTERFACE
            -DNT_PLUGIN # maya dev开发包中指定的不同平台的插件标志(win)
            -D_WIN32
            -DOSWin_
            -DUSERDLL
            -DCRT_SECURE_NO_DEPRECATE
            -D_HAS_ITERATOR_DEBUGGING=0
            -D_CRT_SECURE_NO_WARNINGS
            -DPARTIO_WIN32
    )

    add_library(maya::maya_all ALIAS maya_all)
endif ()
