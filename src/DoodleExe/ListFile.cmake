include(CMakePrintHelpers)
include(${DOODLE_INCLUDE_CMAKE}/CMake/doodle_fun_tool.cmake)

doodle_get_source(
        DOODLELIB_GUI_HEADER
        DOODLELIB_GUI_SOURCE
        LISTS_DIR
        ${CMAKE_CURRENT_LIST_DIR}/Gui/factory
        ${CMAKE_CURRENT_LIST_DIR}/Gui/Metadata
        ${CMAKE_CURRENT_LIST_DIR}/Gui
        ${CMAKE_CURRENT_LIST_DIR}/lib_warp
        ${CMAKE_CURRENT_LIST_DIR}

)

doodle_write_list_file(
        VAR_LIST
        DOODLELIB_GUI_HEADER
        DOODLELIB_GUI_SOURCE
        NAME
        DoodleGui
)
