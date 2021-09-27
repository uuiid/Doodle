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
        kernel/Maya/FbxFile.cpp
        kernel/exception.cpp
        kernel/Maya/Screenshot.cpp
        kernel/Maya/MayaRenderOpenGL.cpp
        kernel/Maya/MayaVideo.cpp
        kernel/ExeWarp/FFmpegWarp.cpp
        kernel/PlayerMotion.cpp

        )

set(doodle_motion_HEADER
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/ExeWarp/FFmpegWarp.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/Maya/MayaRenderOpenGL.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/Maya/FbxFile.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/Maya/MayaVideo.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/Maya/Screenshot.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/BoostUuidWarp.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/Exception.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/MotionFile.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/MotionSetting.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/kernel/PlayerMotion.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/ui/model/MotionModel.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/ui/model/TreeDirItem.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/ui/model/TreeDirModel.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/ui/view/MotionAttrbuteView.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/ui/view/MotionView.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/ui/view/TreeDirView.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/ui/MotionLibWidget.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/ui/MotionMainUI.h
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA/lib/ui/MotionSettingWidget.h
        )

# 由于版本不一样所以使用特制版qt制作maya插件
qt5_wrap_cpp(MAYA_MOC_FILE ${doodle_motion_HEADER})


add_library(doodle_motion SHARED
        ${doodle_motion_CPP}
        ${doodle_motion_HEADER}
        ${MAYA_MOC_FILE}
        )

#设置导出符号
include(GenerateExportHeader)
set(MY_CUSTOM_CONTENT "")
generate_export_header(doodle_motion
        EXPORT_MACRO_NAME DOODLE_MOTION_API
        CUSTOM_CONTENT_FROM_VARIABLE MY_CUSTOM_CONTENT)
#预编译头文件
target_precompile_headers(doodle_motion
        PUBLIC
        <filesystem>
        <variant>
        <string>
        <memory>
        <chrono>
        <map>
        <iostream>
        <regex>
        <fstream>
        <mutex>
        <condition_variable>
        <future>
        <boost/filesystem.hpp>
        )

# 设置一些属性,包括没有自动运行的moc以及安装位置
set_target_properties(doodle_motion PROPERTIES
        AUTOMOC OFF
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
        OUTPUT_NAME doodle_motion_2020
        )


target_include_directories(doodle_motion PUBLIC ${PROJECT_SOURCE_DIR}/include/plug/MAYA)
target_include_directories(doodle_motion PUBLIC
        ${MAYA_INCLUDE_DIR}
        ${OpenCV_INCLUDE_DIRS}
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR}/src/plug/MAYA


        )


target_link_libraries(doodle_motion PUBLIC ${MAYA_LIBRARY} ${MAYA_QT5_LIBRARY})

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


target_compile_definitions(doodle_motion PRIVATE MOTIONGLOBAL_LIBRARY doodle_motion_EXPORT)
target_compile_options(doodle_motion PUBLIC /utf-8)

target_compile_definitions(doodle_motion PRIVATE _WIN32_WINNT=0x0A00)

#调试发布版本需要
# target_compile_options(doodle_motion PRIVATE /Zi)
# target_link_options(doodle_motion PRIVATE /INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF) 

# install(
#   TARGETS doodle_motion
#   DESTINATION plug/maya_plug/plug-ins
# )
# #安装调试文件
# install(FILES $<TARGET_PDB_FILE:doodle_motion>
#         DESTINATION plug/maya_plug/plug-ins
#         # CONFIGURATIONS Debug
#         )
