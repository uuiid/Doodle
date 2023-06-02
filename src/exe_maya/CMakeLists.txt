
add_executable(
        doodle_maya_exe
        main.cpp
)
add_subdirectory(abc)
add_subdirectory(facet)
add_subdirectory(cloth_sim)
add_subdirectory(core)
add_subdirectory(data)

set_target_properties(doodle_maya_exe
        PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
target_link_libraries(doodle_maya_exe
        PUBLIC
        doodle_lib
        doodle_app
        doodle_maya_lib

        maya::maya_all
        DemBones::dem-bones
        Eigen3::Eigen
        OpenMP::OpenMP_CXX
        Alembic::Alembic
)

target_compile_definitions(doodle_maya_exe
        PUBLIC

        # WIN32_LEAN_AND_MEAN
        # ISOLATION_AWARE_ENABLED
        # ENTT_API_IMPORT
)

# target_link_options(doodle_maya_exe PUBLIC /MAP:test.txt)
target_link_options(doodle_maya_exe PUBLIC /ENTRY:mainCRTStartup)
set_target_properties(
        doodle_maya_exe PROPERTIES SUFFIX "_${Maya_Version}.exe"
)

# 定义安装逻辑
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
set(CMAKE_INSTALL_SYSTEM_RUNTIME_COMPONENT maya_plug_com)
include(InstallRequiredSystemLibraries)

install(TARGETS doodle_maya_exe
        RUNTIME_DEPENDENCIES
        DIRECTORIES ${MAYA_BASE_DIR}/bin
        PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "hvsifiletrust" "pdmutilities" "openmaya" "qt" "foundation" "metadata"
        POST_EXCLUDE_REGEXES ".*system32/.*\\.dll" "Maya${Maya_Version}"
        COMPONENT maya_plug_com
)
