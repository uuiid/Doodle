find_package(Maya 2020 REQUIRED)

set(doodle_motion_Mplug_CPP
MotionMayaPlugInit.cpp
MayaPlug.cpp
)

set(doodle_motion_Mplug_HEADER
MotionMayaPlugInit.h
)

add_library(doodle_motion_Mplug MODULE 
  ${doodle_motion_Mplug_CPP}
  ${doodle_motion_Mplug_HEADER}
)

target_include_directories(doodle_motion_Mplug PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)

target_link_libraries(doodle_motion_Mplug PRIVATE doodle_motion)
target_include_directories(doodle_motion_Mplug PRIVATE ${MAYA_INCLUDE_DIR})


#设置maya插件后缀名

set_target_properties(
  doodle_motion_Mplug PROPERTIES SUFFIX ".mll"
)

target_compile_definitions(
  doodle_motion_Mplug 
  PRIVATE
  -DPARTIO_WIN32
  -DNT_PLUGIN 
  -D_WIN32 
  -DOSWin_ 
  -DUSERDLL 
  -DCRT_SECURE_NO_DEPRECATE 
  -D_HAS_ITERATOR_DEBUGGING=0 
  -D_CRT_SECURE_NO_WARNINGS
)

# target_compile_options(
#   doodle_motion_Mplug
#   PRIVATE
#   /MD
# )

#安装基本组件
install(
  TARGETS doodle_motion_Mplug
  DESTINATION plug/maya_plug/plug-ins
)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:doodle_motion_Mplug>
        DESTINATION plug/maya_plug/plug-ins
        CONFIGURATIONS Debug
        )

