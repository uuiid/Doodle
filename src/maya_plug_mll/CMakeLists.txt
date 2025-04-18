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
        doodle_lib
        doodle_maya_lib

        maya::maya_all
        DemBones::dem-bones
        Eigen3::Eigen
        OpenMP::OpenMP_CXX
        Qt5::Core
        Qt5::Widgets
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
)
# 添加fbx动态库复制
add_custom_command(
        TARGET doodle_maya
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:Fbx::Fbx> ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
)

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT maya_plug_com)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION maya/plug-ins)
include(InstallRequiredSystemLibraries)
install(TARGETS doodle_maya
        RUNTIME_DEPENDENCIES
        DIRECTORIES
        ${MAYA_BASE_DIR}/bin
        $<TARGET_FILE_DIR:Fbx::Fbx>
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities" "openmaya" "qt" "foundation" "metadata"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" "Maya${Maya_Version}"
        DESTINATION maya/plug-ins
)

install(DIRECTORY ${PROJECT_SOURCE_DIR}/script/maya/
        DESTINATION maya
)