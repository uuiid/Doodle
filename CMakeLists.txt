cmake_minimum_required(VERSION 3.20)
#添加模块文件
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
include(GetGitVersion)

get_git_version(
        WORK_DIR ${CMAKE_SOURCE_DIR}
)
MESSAGE(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE}")

project(Doodle
        VERSION "${v_short}"
        DESCRIPTION "动画制作中的一些辅助工具"
        LANGUAGES CXX
        HOMEPAGE_URL https://github.com/uuiid/Doodle)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_compile_definitions(QT_NO_KEYWORDS UNICODE _UNICODE NOMINMAX)
#add_compile_options(/W4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


#使用中文
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

#添加选项
option(RUN_QT_TOOL "run qt windeployqt" OFF)
option(RUN_fixup "run fixup " OFF)
option(RUN_COPY "run copy tools" OFF)
option(BUILD_SHARED_LIBS "build doodle lib is SHARED" OFF)
option(Boost_NO_WARN_NEW_VERSIONS "boost warn OFF " ON)
option(USE_RPC_SERVER "use grpc build doodle" OFF)

#首先搜索我们的自定义函数
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/CMake)
include(CMakeRC)

find_package(Boost REQUIRED COMPONENTS
        locale
        iostreams
        program_options
        contract
        filesystem
        coroutine
        )

find_package(Catch2 CONFIG)
find_package(Doxygen)
# 寻找opencv
find_package(OpenCV CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

#仅头库
find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(Sqlpp11 CONFIG REQUIRED
        COMPONENTS
        sqlite3)
find_package(range-v3 CONFIG REQUIRED)
# 压缩库
find_package(cryptopp CONFIG REQUIRED)


# 寻找d3d辅助工具库
find_package(directxtk CONFIG REQUIRED)


add_custom_target(
        update_doc_and_cmrc
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/external/doxygen-awesome-css/doxygen-awesome.css ${PROJECT_SOURCE_DIR}/doc/css
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/external/Font-Awesome/webfonts/fa-brands-400.ttf ${PROJECT_SOURCE_DIR}/src/doodle_lib/resource
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/external/Font-Awesome/webfonts/fa-regular-400.ttf ${PROJECT_SOURCE_DIR}/src/doodle_lib/resource
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/external/Font-Awesome/webfonts/fa-solid-900.ttf ${PROJECT_SOURCE_DIR}/src/doodle_lib/resource
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/external/cmrc/CMakeRC.cmake ${PROJECT_SOURCE_DIR}/CMake/

)

enable_testing()
add_subdirectory(src)


#=====================安装打包过程===============================
# 设置 InstallRequiredSystemLibraries 变量
set(CMAKE_INSTALL_UCRT_LIBRARIES TRUE)
include(InstallRequiredSystemLibraries)

list(APPEND DIRS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin)
add_subdirectory(script)


#最后实现camke修复
install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"$<TARGET_FILE:DoodleExe>\"   \"\"   \"${DIRS}\")
        "
        COMPONENT main_exe
        )
#install(TARGETS DoodleExe
#        COMPONENT main_exe
#        RUNTIME_DEPENDENCIES
#        )

install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/
        DESTINATION bin
        COMPONENT main_exe
        )



set(CPACK_GENERATOR "WIX;External")
set(CPACK_PACKAGE_EXECUTABLES DoodleExe;Doodle)
set(CPACK_CREATE_DESKTOP_LINKS DoodleExe)
set(CPACK_PACKAGE_VENDOR uuiid)
set(CPACK_PACKAGE_INSTALL_DIRECTORY uuiid/doodle${PROJECT_VERSION_MAJOR}${PROJECT_VERSION_MINOR})
set(CPACK_WIX_UPGRADE_GUID 3E490EAC-2FC7-49BF-A521-20F2F93E320A)
set(CPACK_WIX_TEMPLATE ${PROJECT_SOURCE_DIR}/CMake/WIX.template.in)
set(CPACK_ARCHIVE_THREADS 0)
set(CPACK_WIX_PATCH_FILE ${PROJECT_SOURCE_DIR}/CMake/doodle_wix_patch.xml)
set(CPACK_WIX_EXTRA_SOURCES ${PROJECT_SOURCE_DIR}/CMake/doodle_registry.wxs)
set(CPACK_WIX_CULTURES zh-CN)
set(CPACK_EXTERNAL_ENABLE_STAGING True)
set(CPACK_EXTERNAL_PACKAGE_SCRIPT ${PROJECT_SOURCE_DIR}/CMake/cpack_7z_doodle.cmake)

include(CPack REQUIRED)
set(DOODLE_PACKAGE_NAME ${CPACK_PACKAGE_NAME}-${CPACK_PACKAGE_VERSION}-${CPACK_SYSTEM_NAME})


#=====================文档过程===============================
#添加文档生成
set(DOXYGEN_WARNINGS YES)
set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
set(DOXYGEN_WARN_IF_DOC_ERROR YES)
set(DOXYGEN_OUTPUT_LANGUAGE Chinese)
set(DOXYGEN_IMAGE_PATH ${PROJECT_SOURCE_DIR}/doc)
set(DOXYGEN_EXCLUDE
        src/doodle_lib/external
        src/doodle_lib/resource
        src/doodle_lib/generate
        src/doodle_lib/rpc

        src/doodle_exe/main/test
        )
set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
# set(DOXYGEN_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/build)
# set(DOXYGEN_DISABLE_INDEX YES)
set(DOXYGEN_GENERATE_TREEVIEW YES)
# set(DOXYGEN_HTML_INDEX_NUM_ENTRIES 0)
# set(DOXYGEN_HTML_DYNAMIC_SECTIONS YES)
set(DOXYGEN_QUIET YES)
set(DOXYGEN_PROJECT_LOGO src/doodle_exe/main/icon.ico)
set(DOXYGEN_ALIASES
        "doodle_exe_file=${DOODLE_PACKAGE_NAME}"
        )
set(DOXYGEN_NUM_PROC_THREADS 0)
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${PROJECT_SOURCE_DIR}/doc/css/doxygen-awesome.css)
doxygen_add_docs(Doxygen
        src
        doc
        )

#[==[
一些杂项工具和命令

"D:\Program Files\Epic Games\UE_4.25\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Rocket -Plugin=C:\Users\TD\Source\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=C:\Users\TD\Source\Doodle\script\uePlug\4.25\Plugins -VS2019 -CreateSubFolder
"D:\Program Files\Epic Games\UE_4.25\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=C:\Users\TD\Source\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=C:\Users\TD\Source\Doodle\script\uePlug\4.25\Plugins\Doodle -VS2019
"D:\Program Files\Epic Games\UE_4.26\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=C:\Users\TD\Source\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=C:\Users\TD\Source\Doodle\script\uePlug\4.26\Plugins\Doodle -VS2019
"D:\Program Files\Epic Games\UE_4.27\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=C:\Users\TD\Source\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=C:\Users\TD\Source\Doodle\script\uePlug\4.27\Plugins\Doodle -VS2019
$env:VCPKG_DEFAULT_TRIPLET="x64-windows"
install range-v3 eigen3 boost sqlpp11 directxtk catch2 date spdlog cryptopp magic-enum nlohmann-json  opencv[ffmpeg,contrib,freetype] imgui[win32-binding,dx11-binding,docking-experimental] --overlay-ports="C:\Users\TD\Source\Doodle\vcpkg\ports"
imgui[win32-binding,dx11-binding,docking-experimental]:x64-windows  --recurse --overlay-ports="C:\Users\TD\Source\Doodle\vcpkg\ports"
install imguiwrap --overlay-ports="C:\Users\TD\Source\Doodle\vcpkg\ports"
install imgui[win32-binding,dx11-binding,docking-experimental] --overlay-ports="C:\Users\TD\Source\Doodle\vcpkg\ports"
upgrade entt --overlay-ports="C:\Users\TD\Source\Doodle\vcpkg\ports"
upgrade entt --overlay-ports="C:\Users\TD\Source\Doodle\vcpkg\ports" --no-dry-run
sqlpp11[sqlite3-connector] --overlay-ports="C:\Users\TD\Source\Doodle\vcpkg\ports"


"$(SolutionDir)doodle_plug.uproject" -skipcompile -run=DoodleAssCreate -path="E:\Users\TD\Documents\Unreal_Projects\doodle_plug_dev_4.27\test_file\doodle_import_data_main.json"

"$(SolutionDir)doodle_plug.uproject"

    find_package(Simbody CONFIG REQUIRED)
    # note: 1 targets were omitted.
    target_link_libraries(main PRIVATE blas lapack SimTKmath SimTKcommon)
]==]
