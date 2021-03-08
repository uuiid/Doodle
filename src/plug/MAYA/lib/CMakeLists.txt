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


kernel/MotionSetting.cpp
kernel/MotionFile.cpp
kernel/Maya/FbxExport.cpp
kernel/Exception.cpp
kernel/Maya/Screenshot.cpp
kernel/Maya/MayaRenderOpenGL.cpp
kernel/Maya/MayaVideo.h
)

set(doodle_motion_HEADER
kernel/BoostUuidWarp.h
kernel/doodleFFmpeg.h

ui/MotionMainUI.h
ui/MotionSettingWidget.h
ui/MotionLibWidget.h
ui/model/MotionModel.h
ui/model/TreeDirModel.h
ui/model/TreeDirItem.h
ui/view/MotionView.h
ui/view/TreeDirView.h

kernel/MotionSetting.h
kernel/MotionFile.h
kernel/Maya/FbxExport.h
kernel/Exception.h
kernel/Maya/Screenshot.h
kernel/Maya/MayaRenderOpenGL.h
kernel/Maya/MayaVideo.cpp
)





add_library(doodle_motion SHARED 
  MotionGlobal.h
  ${doodle_motion_CPP}
  ${doodle_motion_HEADER}
)

target_include_directories(doodle_motion PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)


target_link_libraries(doodle_motion PUBLIC ${MAYA_LIBRARY} ${MAYA_QT5_LIBRARY})
target_link_libraries(doodle_motion PRIVATE nlohmann_json nlohmann_json::nlohmann_json)
#链接fbx库
target_link_libraries(doodle_motion PRIVATE 
Autodesk_FBX
Boost::locale
${FFMPEG_LIBRARIES}
)

# target_link_libraries(doodle_motion PRIVATE SqlppSqlite)

target_include_directories(doodle_motion PRIVATE ${MAYA_INCLUDE_DIR})

target_compile_definitions(doodle_motion PRIVATE MOTIONGLOBAL_LIBRARY)
target_compile_options(doodle_motion PUBLIC /utf-8)


#调试发布版本需要
target_compile_options(doodle_motion PRIVATE /Zi)
target_link_options(doodle_motion PRIVATE /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF) 

install(
  TARGETS doodle_motion
  DESTINATION plug/maya_plug/plug-ins
)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:doodle_motion>
        DESTINATION plug/maya_plug/plug-ins
        # CONFIGURATIONS Debug
        )

