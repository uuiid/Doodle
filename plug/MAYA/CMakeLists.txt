set(MAYA_DEFAULT_LOCATION "F:/src/maya2020_dev/devkitBase")


####################################
set(MAYA_VERSION 2020)

if(WIN32)
  set(MAYA_DEFAULT_LOCATION "C:/Program Files/Autodesk/Maya${MAYA_VERSION}")
  set(OPEN_MAYA OpenMaya.lib)
endif()

unset(MAYA_BASE_DIR CACHE )
unset(MAYA_INCLUDE_DIR CACHE )
unset(MAYA_LIBRARY_DIR CACHE )

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

if(MAYA_INCLUDE_DIR AND MAYA_LIBRARY_DIR)
  message("maya 找到开发组件 \"${MAYA_INCLUDE_DIR}\" \"${MAYA_LIBRARY_DIR}\"")
endif()