
#[[
寻找 adlobe Photoshop Sdk




#]]


include(CMakePrintHelpers)

find_path(
  PHOTOSHOP_PATH
  pluginsdk/photoshopapi/photoshop/PIAbout.h
  HINS
  "${PHOTOSHOP}"
  "$ENV{PHOTOSHOP}"
  "${Photoshop_ROOT_DIR}"
  "[HKLM\\SOFTWARE\\Adobe\\Photoshop\\${Photoshop_FIND_VERSION}\\ApplicationPath]"
  DOC
  "adlobe Photoshop 安装路径"
)

find_path(
  PHOTOSHOP_PATH_SDK
  PIAbout.h
  HINS
  "${PHOTOSHOP_PATH}/pluginsdk/photoshopapi/photoshop"
  "${Photoshop_sdk_ROOT_DIR}/pluginsdk/photoshopapi/photoshop"
  DOC
  "adlobe Photoshop 头文件安装路径"
)

find_program(
  PHOTOSHOP_CNVTPIPL_SDK
  pluginsdk/samplecode/resources/Cnvtpipl.exe
  HINS
  "${PHOTOSHOP_PATH}"
  "${Photoshop_sdk_ROOT_DIR}"
  DOC
  "adlobe Photoshop 辅助生成windows资源工具"
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Photoshop
REQUIRED_VARS
PHOTOSHOP_PATH_SDK PHOTOSHOP_CNVTPIPL_SDK
REASON_FAILURE_MESSAGE "没有找到 Photoshop Cnvtpipl.exe文件路径"
)

if(Photoshop_FOUND)
  add_library(Photoshop INTERFACE)
  add_executable(Cnvtpipl IMPORTED)
  set_target_properties(Cnvtpipl PROPERTIES 
    IMPORTED_LOCATION ${PHOTOSHOP_CNVTPIPL_SDK}
  )

  target_include_directories(Photoshop
  INTERFACE
  "${PHOTOSHOP_PATH_SDK}"
  )
  add_library(Photoshop::Photoshop ALIAS Photoshop)
  add_executable(Photoshop::Cnvtpipl ALIAS Cnvtpipl)
  
endif()