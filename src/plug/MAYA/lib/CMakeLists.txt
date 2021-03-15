find_package(Maya 2020 REQUIRED)

# helper macro to set up a moc rule
function(Maya_qt5_create_moc_command infile outfile moc_flags moc_options moc_target moc_depends)
    _qt5_warn_deprecated("Maya_qt5_create_moc_command")

    # Pass the parameters in a file.  Set the working directory to
    # be that containing the parameters file and reference it by
    # just the file name.  This is necessary because the moc tool on
    # MinGW builds does not seem to handle spaces in the path to the
    # file given with the @ syntax.
    get_filename_component(_moc_outfile_name "${outfile}" NAME)
    get_filename_component(_moc_outfile_dir "${outfile}" PATH)
    if(_moc_outfile_dir)
        set(_moc_working_dir WORKING_DIRECTORY ${_moc_outfile_dir})
    endif()
    set (_moc_parameters_file ${outfile}_parameters)
    set (_moc_parameters ${moc_flags} ${moc_options} -o "${outfile}" "${infile}")
    string (REPLACE ";" "\n" _moc_parameters "${_moc_parameters}")

    if(moc_target)
        set(_moc_parameters_file ${_moc_parameters_file}$<$<BOOL:$<CONFIGURATION>>:_$<CONFIGURATION>>)
        set(targetincludes "$<TARGET_PROPERTY:${moc_target},INCLUDE_DIRECTORIES>")
        set(targetdefines "$<TARGET_PROPERTY:${moc_target},COMPILE_DEFINITIONS>")

        set(targetincludes "$<$<BOOL:${targetincludes}>:-I$<JOIN:${targetincludes},\n-I>\n>")
        set(targetdefines "$<$<BOOL:${targetdefines}>:-D$<JOIN:${targetdefines},\n-D>\n>")

        file (GENERATE
            OUTPUT ${_moc_parameters_file}
            CONTENT "${targetdefines}${targetincludes}${_moc_parameters}\n"
        )

        set(targetincludes)
        set(targetdefines)
    else()
        file(WRITE ${_moc_parameters_file} "${_moc_parameters}\n")
    endif()

    set(_moc_extra_parameters_file @${_moc_parameters_file})
    add_custom_command(OUTPUT ${outfile}
                       COMMAND "C:/Program Files/Autodesk/Maya2020/bin/moc.exe" ${_moc_extra_parameters_file}
                       DEPENDS ${infile} ${moc_depends}
                       ${_moc_working_dir}
                       VERBATIM)
    set_source_files_properties(${infile} PROPERTIES SKIP_AUTOMOC ON)
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOMOC ON)
    set_source_files_properties(${outfile} PROPERTIES SKIP_AUTOUIC ON)
endfunction()

function(Maya_qt5_wrap_cpp outfiles)
    set(_QT5_INTERNAL_SCOPE ON)

    # get include dirs
    qt5_get_moc_flags(moc_flags)

    set(options)
    set(oneValueArgs TARGET)
    set(multiValueArgs OPTIONS DEPENDS)

    cmake_parse_arguments(_WRAP_CPP "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(moc_files ${_WRAP_CPP_UNPARSED_ARGUMENTS})
    set(moc_options ${_WRAP_CPP_OPTIONS})
    set(moc_target ${_WRAP_CPP_TARGET})
    set(moc_depends ${_WRAP_CPP_DEPENDS})

    foreach(it ${moc_files})
        get_filename_component(it ${it} ABSOLUTE)
        qt5_make_output_file(${it} moc_ cpp outfile)
        Maya_qt5_create_moc_command(${it} ${outfile} "${moc_flags}" "${moc_options}" "${moc_target}" "${moc_depends}")
        list(APPEND ${outfiles} ${outfile})
    endforeach()
    set(${outfiles} ${${outfiles}} PARENT_SCOPE)
endfunction()


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
kernel/Maya/MayaVideo.cpp
kernel/ExeWarp/FFmpegWarp.cpp
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

kernel/MotionSetting.h
kernel/MotionFile.h
kernel/Maya/FbxExport.h
kernel/Exception.h
kernel/Maya/Screenshot.h
kernel/Maya/MayaRenderOpenGL.h
kernel/Maya/MayaVideo.h
kernel/ExeWarp/FFmpegWarp.h
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

