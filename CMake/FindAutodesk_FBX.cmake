# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindFBX
-------

Finds the FBX library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``FBX::FBX``
  The FBX library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``FBX_FOUND``
  True if the system has the FBX library.
``FBX_VERSION``
  The version of the FBX library which was found.
``FBX_INCLUDE_DIRS``
  Include directories needed to use FBX.
``FBX_LIBRARIES``
  Libraries needed to link to FBX.

Cache Variables
^^^^^^^^^^^^^^^

The following cache variables may also be set:

``FBX_INCLUDE_DIR``
  The directory containing ``FBX.h``.
``FBX_LIBRARY``
  The path to the FBX library.

#]=======================================================================]


if(WIN32)
  set(MAYA_FBX_LOCATION "C:/Program Files/Autodesk/FBX/FBX SDK/${Autodesk_FBX_FIND_VERSION}.0.1")
endif()

find_path(FBX_BASE_DIR
  include/fbxsdk.h
  HINTS
  ${MAYA_FBX_LOCATION}
  DOC
    "fbx sdk 基本路径"
)

find_path(FBX_INCLUDE_DIR
  fbxsdk.h
  HINTS
  ${MAYA_FBX_LOCATION}/include
  DOC
    "fbx sdk include 路径"
    )

find_library(FBX_LIBRARY
  lib/vs2017/x64/${CMAKE_BUILD_TYPE}/libfbxsdk-md.lib
  HINTS
  ${MAYA_FBX_LOCATION}
  DOC
    "fbx 链接库路径 ${CMAKE_BUILD_TYPE}"
)

include(FindPackageHandleStandardArgs)


find_package_handle_standard_args(Autodesk_FBX
    FOUND_VAR 
        Autodesk_FBX_FOUND
    REQUIRED_VARS
        FBX_INCLUDE_DIR
        FBX_LIBRARY
    REASON_FAILURE_MESSAGE 
        "fbx没有找到"
)


add_library(Autodesk_FBX STATIC IMPORTED)

set_target_properties(Autodesk_FBX PROPERTIES
IMPORTED_LOCATION "${FBX_LIBRARY}"
INTERFACE_INCLUDE_DIRECTORIES "${FBX_INCLUDE_DIR}"
)
add_library(maya::Fbx ALIAS Autodesk_FBX)