#[=======================================================================[.rst:
FindWix
-------

寻找 wix 工具集

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
`` maya_all ``
  The path to the Maya library.

#]=======================================================================]

find_program(
        ${CMAKE_FIND_PACKAGE_NAME}_EXECUTABLE
        NAMES wix

        HINTS
        $ENV{USERPROFILE}/.dotnet/tools

        PATH_SUFFIXES
        bin
        DOC "wix exe工具"
)

include(FindPackageHandleStandardArgs)
# 添加约束
find_package_handle_standard_args(
        ${CMAKE_FIND_PACKAGE_NAME}
        REQUIRED_VARS
        ${CMAKE_FIND_PACKAGE_NAME}_EXECUTABLE
        REASON_FAILURE_MESSAGE "没有找到 wix 工具集"
)


add_executable(wix_exe IMPORTED GLOBAL)
set_target_properties(
        wix_exe
        PROPERTIES
        IMPORTED_LOCATION ${${CMAKE_FIND_PACKAGE_NAME}_EXECUTABLE}
)

