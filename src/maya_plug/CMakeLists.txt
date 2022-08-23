
if (DEFINED Maya_Version)
    find_package(Maya ${Maya_Version} MODULE)
    find_package(OpenMP REQUIRED)

    cmake_print_variables(MAYA_DEFAULT_LOCATION)
    #    set(CMAKE_MAP_IMPORTED_CONFIG_DEBUG Release)
    #    set(CMAKE_MAP_IMPORTED_CONFIG_RELEASE Release)
    #    set(CMAKE_MAP_IMPORTED_CONFIG_RELWITHDEBINFO Release)
    #    set(CMAKE_MAP_IMPORTED_CONFIG_MINSIZEREL Release)
    find_package(Qt5 COMPONENTS Core Widgets REQUIRED
            HINTS ${MAYA_DEFAULT_LOCATION})
    find_package(dem-bones CONFIG REQUIRED)


    if (TARGET maya::maya_all)
        set(maya_plug_name doodle_maya)
        configure_file(DoodleExe.manifest.in doodle_maya.manifest)
        configure_file(maya_plug.rc.in doodle_maya.rc)
        add_library(doodle_maya MODULE
                doodle_maya.manifest
                doodle_maya.rc
                ../doodle_exe/main/icon.ico
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
        target_link_libraries(doodle_maya PRIVATE doodle_lib
                maya::maya_all
                Qt5::Core
                Qt5::Widgets
                DemBones::dem-bones
                Eigen3::Eigen
                OpenMP::OpenMP_CXX
                )
        target_include_directories(doodle_maya PUBLIC ../doodle_exe)
        #设置maya插件后缀名
        set_target_properties(
                doodle_maya PROPERTIES SUFFIX "_${Maya_Version}.mll"
        )


        configure_file(doodle.mod.in ${PROJECT_BINARY_DIR}/plug/maya/doodle.mod)
        file(GLOB_RECURSE maya_script_files FOLLOW_SYMLINKS
                LIST_DIRECTORIES false
                CONFIGURE_DEPENDS
                "${PROJECT_SOURCE_DIR}/script/maya/*.*")
#        message("${maya_script_files}")
        add_custom_command(
                TARGET doodle_maya
                POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/script/maya ${PROJECT_BINARY_DIR}/plug/maya
                COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_LIST_DIR}/../doodle_core/resource/maya_fun_tool.py ${PROJECT_BINARY_DIR}/plug/maya/scripts/scripts/
                DEPENDS ${maya_script_files}
        )
        # 定义安装逻辑
        install(TARGETS doodle_maya
                DESTINATION maya/plug-ins
                COMPONENT maya_plug_com
                )

        install(DIRECTORY ${PROJECT_SOURCE_DIR}/script/maya/
                DESTINATION maya
                COMPONENT maya_plug_com)
        install(FILES ${CMAKE_CURRENT_LIST_DIR}/../doodle_core/resource/maya_fun_tool.py
                DESTINATION maya/scripts/scripts/
                COMPONENT maya_plug_com)


    endif ()

endif ()

