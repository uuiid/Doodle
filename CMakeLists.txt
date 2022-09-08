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
option(BUILD_SHARED_LIBS "build doodle lib is SHARED" OFF)
option(Boost_NO_WARN_NEW_VERSIONS "boost warn OFF " ON)


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
        unit_test_framework
        )

find_package(Catch2 CONFIG)
find_package(Doxygen)
# 寻找opencv
find_package(OpenCV CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
# 寻找 openssl
find_package(OpenSSL REQUIRED)
#仅头库
find_package(magic_enum CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(EnTT CONFIG REQUIRED)
find_package(Sqlpp11 CONFIG REQUIRED
        COMPONENTS
        sqlite3)
find_package(Eigen3 CONFIG REQUIRED)
find_package(range-v3 CONFIG REQUIRED)
# this is heuristically generated, and may not be correct
find_package(implot CONFIG REQUIRED)
#target_link_libraries(main PRIVATE implot::implot)
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
set(DOODLE_PACKAGE_NAME ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})
cmake_print_variables(DOODLE_PACKAGE_NAME)
add_subdirectory(src)


#=====================安装打包过程===============================
add_subdirectory(script)


#=====================文档过程===============================
#添加文档生成
set(DOXYGEN_WARNINGS YES)
set(DOXYGEN_WARN_IF_UNDOCUMENTED YES)
set(DOXYGEN_WARN_IF_DOC_ERROR YES)
set(DOXYGEN_OUTPUT_LANGUAGE Chinese)
set(DOXYGEN_EXCLUDE
        src/doodle_lib/external
        src/doodle_lib/resource
        src/doodle_core/generate
        src/doodle_lib/rpc
        src/test
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
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${PROJECT_SOURCE_DIR}/docs/css/doxygen-awesome.css)
doxygen_add_docs(Doxygen
        src
        )

#[==[
一些杂项工具和命令

"D:\Program Files\Epic Games\UE_4.25\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Rocket -Plugin=C:\Users\TD\Source\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=C:\Users\TD\Source\Doodle\script\uePlug\4.25\Plugins -VS2019 -CreateSubFolder
"D:\Program Files\Epic Games\UE_4.25\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=C:\Users\TD\Source\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=C:\Users\TD\Source\Doodle\script\uePlug\4.25\Plugins\Doodle -VS2019
"D:\Program Files\Epic Games\UE_4.26\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=C:\Users\TD\Source\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=C:\Users\TD\Source\Doodle\script\uePlug\4.26\Plugins\Doodle -VS2019
"D:\Program Files\Epic Games\UE_4.27\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=C:\Users\TD\Source\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=C:\Users\TD\Source\Doodle\script\uePlug\4.27\Plugins\Doodle -VS2019
$env:VCPKG_DEFAULT_TRIPLET="x64-windows"
install range-v3 eigen3 boost sqlpp11 directxtk catch2 date spdlog cryptopp magic-enum nlohmann-json winreg opencv[ffmpeg,contrib,freetype] imgui[win32-binding,dx11-binding,docking-experimental] --overlay-ports="C:\Users\TD\Source\Doodle\vcpkg\ports"
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

        find_package(dem-bones CONFIG REQUIRED)
    target_link_libraries(main PRIVATE DemBones::dem-bones)

]==]
