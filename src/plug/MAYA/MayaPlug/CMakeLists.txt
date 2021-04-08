find_package(Maya 2020 REQUIRED)

set(doodle_motion_Mplug_CPP
MotionMayaPlugInit.cpp
MayaPlug.cpp
)

set(doodle_motion_Mplug_HEADER

)

add_library(doodle_motion_Mplug MODULE 
  ${doodle_motion_Mplug_CPP}
  ${doodle_motion_Mplug_HEADER}
)
set_target_properties(doodle_motion_Mplug PROPERTIES 
RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
)
target_include_directories(doodle_motion_Mplug PUBLIC ${PROJECT_SOURCE_DIR}/include/plug/MAYA)

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
