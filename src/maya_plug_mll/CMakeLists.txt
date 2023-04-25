set(maya_plug_name doodle_maya)

add_library(doodle_maya MODULE main.cpp)


target_include_directories(doodle_maya PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
set_target_properties(doodle_maya PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
        )
target_link_libraries(doodle_maya
        PRIVATE
        doodle_core
        doodle_app
        doodle_maya_lib

        maya::maya_all
        DemBones::dem-bones
        Eigen3::Eigen
        OpenMP::OpenMP_CXX
        )
target_compile_options(
        doodle_maya
        PRIVATE

        $<$<CXX_COMPILER_ID:MSVC>:/MP>
        $<$<CXX_COMPILER_ID:MSVC>:/bigobj>
)

# 设置maya插件后缀名
set_target_properties(
        doodle_maya PROPERTIES SUFFIX "_${Maya_Version}.mll"
)

configure_file(doodle.mod.in ${PROJECT_BINARY_DIR}/plug/maya/doodle.mod)


add_custom_command(
        TARGET doodle_maya
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/script/maya ${PROJECT_BINARY_DIR}/plug/maya
        COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../doodle_core/resource/maya_fun_tool.py ${PROJECT_BINARY_DIR}/plug/maya/scripts/scripts/
)


# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT maya_plug_com)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION maya/plug-ins)
include(InstallRequiredSystemLibraries)
install(TARGETS doodle_maya
        RUNTIME_DEPENDENCIES
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities" "openmaya" "qt" "foundation" "metadata"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" "Maya${Maya_Version}"
        DESTINATION maya/plug-ins
        COMPONENT maya_plug_com
        )

install(DIRECTORY ${PROJECT_SOURCE_DIR}/script/maya/
        DESTINATION maya
        COMPONENT maya_plug_com)
install(FILES ${CMAKE_CURRENT_LIST_DIR}/../doodle_core/resource/maya_fun_tool.py
        DESTINATION maya/scripts/scripts/
        COMPONENT maya_plug_com)
install(FILES maya_plug_install.wxs
        DESTINATION wix
        COMPONENT maya_plug_com)
doodle_install_code_wix(maya maya_plug_com)