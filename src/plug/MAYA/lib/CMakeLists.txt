find_package(Maya 2020 REQUIRED)

set(doodle_motion_CPP
ui/MotionMainUI.cpp
ui/MotionSettingWidget.cpp
ui/MotionLibWidget.cpp
ui/model/MotionModel.cpp
ui/model/TreeDirModel.cpp
ui/model/TreeDirItem.cpp
ui/view/MotionView.cpp
ui/view/TreeDirView.cpp
ui/view/MotionAttrbuteView.cpp


kernel/MotionSetting.cpp
kernel/MotionFile.cpp
kernel/Maya/FbxExport.cpp
kernel/Exception.cpp
kernel/Maya/Screenshot.cpp
kernel/Maya/MayaRenderOpenGL.cpp
kernel/Maya/MayaVideo.cpp
kernel/ExeWarp/FFmpegWarp.cpp
kernel/PlayerMotion.cpp

)

set(doodle_motion_HEADER
kernel/BoostUuidWarp.h

ui/MotionMainUI.h
ui/MotionSettingWidget.h
ui/MotionLibWidget.h
ui/model/MotionModel.h
ui/model/TreeDirModel.h
ui/model/TreeDirItem.h
ui/view/MotionView.h
ui/view/TreeDirView.h
ui/view/MotionAttrbuteView.h

kernel/MotionSetting.h
kernel/MotionFile.h
kernel/Maya/FbxExport.h
kernel/Exception.h
kernel/Maya/Screenshot.h
kernel/Maya/MayaRenderOpenGL.h
kernel/Maya/MayaVideo.h
kernel/ExeWarp/FFmpegWarp.h
kernel/PlayerMotion.h

)

# 由于版本不一样所以使用特制版qt制作maya插件
Maya_qt5_wrap_cpp(MAYA_MOC_FILE ${doodle_motion_HEADER} )


add_library(doodle_motion SHARED 
  MotionGlobal.h
  ${doodle_motion_CPP}
  ${doodle_motion_HEADER}
  ${MAYA_MOC_FILE}
)
# 设置一些属性,包括没有自动运行的moc以及安装位置
set_target_properties(doodle_motion PROPERTIES 
AUTOMOC OFF
RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
)




target_include_directories(doodle_motion PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_include_directories(doodle_motion PUBLIC 
${MAYA_INCLUDE_DIR}
${OpenCV_INCLUDE_DIRS}
)


target_link_libraries(doodle_motion PUBLIC ${MAYA_LIBRARY} ${MAYA_QT5_LIBRARY})
target_link_libraries(doodle_motion PRIVATE )
#链接fbx库
target_link_libraries(doodle_motion PRIVATE 
Autodesk_FBX
Boost::locale
Boost::filesystem
${OpenCV_LIBS}
nlohmann_json 
nlohmann_json::nlohmann_json
# ${FFMPEG_LIBRARIES}
)

# target_link_libraries(doodle_motion PRIVATE SqlppSqlite)


target_compile_definitions(doodle_motion PRIVATE MOTIONGLOBAL_LIBRARY)
target_compile_options(doodle_motion PUBLIC /utf-8)

target_compile_definitions(doodle_motion PRIVATE _WIN32_WINNT=0x0A00)

#调试发布版本需要
target_compile_options(doodle_motion PRIVATE /Zi)
target_link_options(doodle_motion PRIVATE /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF) 

# install(
#   TARGETS doodle_motion
#   DESTINATION plug/maya_plug/plug-ins
# )
# #安装调试文件
# install(FILES $<TARGET_PDB_FILE:doodle_motion>
#         DESTINATION plug/maya_plug/plug-ins
#         # CONFIGURATIONS Debug
#         )

