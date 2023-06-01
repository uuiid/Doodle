#[=======================================================================[.rst:
FindInno
-------

寻找 innosetup 工具集

#]=======================================================================]

find_program(
        ${CMAKE_FIND_PACKAGE_NAME}_EXECUTABLE
        NAMES ISCC

        HINTS
        "C:/Program Files (x86)/Inno Setup 6"

        PATH_SUFFIXES
        bin
        DOC "inno setup工具"
)

include(FindPackageHandleStandardArgs)

# 添加约束
find_package_handle_standard_args(
        ${CMAKE_FIND_PACKAGE_NAME}
        REQUIRED_VARS
        ${CMAKE_FIND_PACKAGE_NAME}_EXECUTABLE
        REASON_FAILURE_MESSAGE "没有找到 wix 工具集"
)

add_executable(${CMAKE_FIND_PACKAGE_NAME}_exe IMPORTED GLOBAL)
set_target_properties(
        ${CMAKE_FIND_PACKAGE_NAME}_exe
        PROPERTIES
        IMPORTED_LOCATION ${${CMAKE_FIND_PACKAGE_NAME}_EXECUTABLE}
)
