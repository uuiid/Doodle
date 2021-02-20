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

# set(MAYA_INCLUDE_DIR "" CACHE PATH "maya头文件目录")
# set(MAYA_LIBRARY "" CACHE PATH "maya连接库目录")

if(WIN32)
  set(MAYA_DEFAULT_LOCATION "C:/Program Files/Autodesk/Maya${Maya_FIND_VERSION}")
  set(OPEN_MAYA OpenMaya.lib)
endif()

# unset(MAYA_BASE_DIR CACHE )
# unset(MAYA_INCLUDE_DIR CACHE )
# unset(MAYA_LIBRARY_DIR CACHE )

find_path(MAYA_BASE_DIR
  include/maya/MFn.h
  HINTS
    ${MAYA_DEFAULT_LOCATION}
  DOC
   "maya 基本路径"
)

find_path(MAYA_INCLUDE_DIR
  maya/MFn.h

  HINTS
    ${MAYA_DEFAULT_LOCATION}
  PATH_SUFFIXES
    "include/"
    REQUIRED
  DOC
   "maya 导入路径"
)

find_path(MAYA_LIBRARY_DIR
        ${OPEN_MAYA}

        HINTS
          ${MAYA_DEFAULT_LOCATION}
        PATH_SUFFIXES
          "lib/"
          REQUIRED
        DOC
          "maya 连接库"
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
    IMFbase
    cg
    cgGL
    clew
)

set(MAYA_LIBS_TO_QT_FIND
Qt53DAnimation
Qt53DCore
Qt53DExtras
Qt53DInput
Qt53DLogic
Qt53DQuick
Qt53DQuickAnimation
Qt53DQuickExtras
Qt53DQuickInput
Qt53DQuickRender
Qt53DQuickScene2D
Qt53DRender
Qt5AxBase
Qt5AxContainer
Qt5AxServer
Qt5Bluetooth
Qt5Concurrent
Qt5Core
Qt5DBus
Qt5Gamepad
Qt5Gui
Qt5Help
Qt5Location
Qt5Multimedia
Qt5MultimediaQuick
Qt5MultimediaWidgets
Qt5Network
Qt5NetworkAuth
Qt5Nfc
Qt5OpenGL
Qt5OpenGLExtensions
Qt5Positioning
Qt5PositioningQuick
Qt5PrintSupport
Qt5Purchasing
Qt5Qml
Qt5Quick
Qt5QuickControls2
Qt5QuickParticles
Qt5QuickShapes
Qt5QuickTemplates2
Qt5QuickTest
Qt5QuickWidgets
Qt5RemoteObjects
Qt5Scxml
Qt5Sensors
Qt5SerialBus
Qt5SerialPort
Qt5Sql
Qt5Svg
Qt5Test
Qt5TextToSpeech
Qt5ThemeSupport
Qt5UiTools
Qt5WebChannel
Qt5WebEngine
Qt5WebEngineCore
Qt5WebEngineWidgets
Qt5WebSockets
Qt5WebView
Qt5Widgets
Qt5WindowsUIAutomationSupport
Qt5WinExtras
Qt5Xml
Qt5XmlPatterns
)

foreach(MAYA_LIB ${MAYA_LIBS_TO_FIND})
  find_library(MAYA_${MAYA_LIB}_LIBRARY
    ${MAYA_LIB}
    HINTS
     ${MAYA_LIBRARY_DIR}
    REQUIRED
    DOC
     "寻找maya ${MAYA_LIB}库"
     NO_CMAKE_SYSTEM_PATH
  )

  if(MAYA_${MAYA_LIB}_LIBRARY)
    list(APPEND MAYA_LIBRARY ${MAYA_${MAYA_LIB}_LIBRARY})
  endif()
endforeach()

foreach(MAYA_QT5_LIB ${MAYA_LIBS_TO_QT_FIND})
  find_library(MAYA_QT_${MAYA_QT5_LIB}_LIBRARY
  ${MAYA_QT5_LIB}
  HINTS
   ${MAYA_LIBRARY_DIR}
  REQUIRED
  DOC
   "寻找maya qt ${MAYA_QT5_LIB} 组件"
  NO_CMAKE_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_CMAKE_SYSTEM_PATH
  NO_DEFAULT_PATH
  )
  if(MAYA_QT_${MAYA_QT5_LIB}_LIBRARY)
    list(APPEND MAYA_QT5_LIBRARY ${MAYA_QT_${MAYA_QT5_LIB}_LIBRARY})
  endif()
endforeach()



include(FindPackageHandleStandardArgs)


find_package_handle_standard_args(Maya
    FOUND_VAR 
        Maya_FOUND
    REQUIRED_VARS
        MAYA_INCLUDE_DIR
        MAYA_LIBRARY
        MAYA_QT5_LIBRARY
    REASON_FAILURE_MESSAGE 
        "maya没有找到"
)


