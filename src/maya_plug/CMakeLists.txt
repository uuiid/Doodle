# 寻找 maya 包(包括各种动态库) 会在 ${CMAKE_PREFIX_PATH} 这里找 -> 找到 `FindMaya.cmake`
# 模块的寻找文件名称 Find + 包名称 + .cmake
# 开始运行 `FindMaya.cmake`
find_package(Maya ${Maya_Version} MODULE)
find_package(Autodesk_FBX 2020 MODULE)
find_package(OpenMP REQUIRED)

cmake_print_variables(MAYA_BASE_DIR)

# set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)
# set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE Release)
# set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
# set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED
        HINTS ${MAYA_BASE_DIR})
find_package(dem-bones CONFIG REQUIRED)

if (TARGET maya::maya_all)
    set(maya_plug_name doodle_maya)
    configure_file(DoodleExe.manifest.in doodle_maya.manifest)
    configure_file(maya_plug.rc.in doodle_maya.rc)
    add_library(doodle_maya MODULE
            doodle_maya.manifest
            doodle_maya.rc
            ${PROJECT_SOURCE_DIR}/src/icon.ico
            )

    add_subdirectory(configure configure_${Maya_Version})
    add_subdirectory(data data_${Maya_Version})
    add_subdirectory(exception exception_${Maya_Version})
    add_subdirectory(fmt fmt_${Maya_Version})
    add_subdirectory(gui gui_${Maya_Version})
    add_subdirectory(logger logger_${Maya_Version})
    add_subdirectory(main main_${Maya_Version})
    add_subdirectory(maya_comm maya_comm_${Maya_Version})
    add_subdirectory(maya_render maya_render_${Maya_Version})

    target_include_directories(doodle_maya PUBLIC ${CMAKE_CURRENT_BINARY_DIR})
    set_target_properties(doodle_maya PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
            LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/plug/maya/plug-ins
            )
    target_link_libraries(doodle_maya
            PRIVATE
            doodle_core
            doodle_app
            maya::maya_all
            Qt5::Core
            Qt5::Widgets
            DemBones::dem-bones
            Eigen3::Eigen
            OpenMP::OpenMP_CXX
            )

    # 设置maya插件后缀名
    set_target_properties(
            doodle_maya PROPERTIES SUFFIX "_${Maya_Version}.mll"
    )

    configure_file(doodle.mod.in ${PROJECT_BINARY_DIR}/plug/maya/doodle.mod)

    list(APPEND maya_script_files ${CMAKE_CURRENT_LIST_DIR}/../doodle_core/resource/maya_fun_tool.py)

    # message("${maya_script_files}")
    add_custom_target(
            doodle_maya_python_files
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/script/maya ${PROJECT_BINARY_DIR}/plug/maya
            COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../doodle_core/resource/maya_fun_tool.py ${PROJECT_BINARY_DIR}/plug/maya/scripts/scripts/
    )

    add_dependencies(doodle_maya doodle_maya_python_files)

    # 定义安装逻辑
    #                set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
    #                set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT maya_plug_com)
    #                include(InstallRequiredSystemLibraries)
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

endif ()