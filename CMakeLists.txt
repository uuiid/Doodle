# 确定最低版本
cmake_minimum_required(VERSION 3.20)

# 添加模块文件(辅助cmake寻找需要的动态库)
# CMAKE_SOURCE_DIR 代表当前文件的路径
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")
option(ADD_VERSION_LABEL "添加git版本标签和递增版本号" OFF)
# 寻找git版本标签🏷️
include(GetGitVersion)
get_git_version(
        WORK_DIR ${CMAKE_SOURCE_DIR}
        ADD_LABEL ${ADD_VERSION_LABEL}
)

MESSAGE(STATUS "Using toolchain file: ${CMAKE_TOOLCHAIN_FILE} OS SDK ${CMAKE_SYSTEM_VERSION}")

# 创建一个项目
project(Doodle
        VERSION "${v_short}"
        DESCRIPTION "动画制作中的一些辅助工具"
        LANGUAGES CXX
        HOMEPAGE_URL https://github.com/uuiid/Doodle)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 项目中所有的 std::string 类都是utf-8字节  认为 char -> char8
add_compile_definitions(QT_NO_KEYWORDS UNICODE _UNICODE NOMINMAX)

# add_compile_options(/W4)
# message("CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_LIST_DIR}/external/debug/libtorch)
else ()
    list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_LIST_DIR}/external/libtorch)
endif ()

# 设置 c++ 版本
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 使用中文
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

# 添加选项
option(BUILD_SHARED_LIBS "build doodle lib is SHARED" OFF)
option(BUILD_LIMITED_1 "受限构建" OFF)
option(BUILD_USD "构建usd相关" OFF)

# 首先搜索我们的自定义函数
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/CMake)

# 寻找包
find_package(Boost REQUIRED COMPONENTS
        locale
        iostreams
        program_options
        contract
        filesystem
        coroutine
        unit_test_framework
        log
        log_setup
        )

find_package(Doxygen)

# 寻找 opencv
find_package(OpenCV CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)

# 寻找 openssl
find_package(OpenSSL REQUIRED)
# 寻找 表格库
find_package(Xlnt CONFIG REQUIRED)

# 仅头库
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
find_package(wil CONFIG REQUIRED)

# this is heuristically generated, and may not be correct
find_package(implot CONFIG REQUIRED)

# 寻找反射
find_package(rttr CONFIG REQUIRED)

# 压缩库
find_package(cryptopp CONFIG REQUIRED)

# 寻找abc库
find_package(Alembic CONFIG REQUIRED)

# 寻找d3d辅助工具库
#find_package(directxtk CONFIG REQUIRED)

# 启用测试
enable_testing()
set(DOODLE_PACKAGE_NAME ${CMAKE_PROJECT_NAME}-${CMAKE_PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR})

# =====================安装打包过程===============================
include(doodle_exe)

if (BUILD_USD)
    include(build_usd)
endif ()

#include(CPack)
#set(CPACK_EXTERNAL_ENABLE_STAGING TRUE)
add_subdirectory(script)
add_subdirectory(src)

#[==[
一些杂项工具和命令
"D:\Program Files\Epic Games\UE_4.25\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Rocket -Plugin=E:\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=E:\Doodle\script\uePlug\4.25\Plugins -VS2019 -CreateSubFolder
"D:\Program Files\Epic Games\UE_4.25\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=E:\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=E:\Doodle\script\uePlug\4.25\Plugins\Doodle -VS2019
"D:\Program Files\Epic Games\UE_4.26\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=E:\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=E:\Doodle\script\uePlug\4.26\Plugins\Doodle -VS2019
"D:\Program Files\Epic Games\UE_4.27\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=E:\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=E:\Doodle\script\uePlug\4.27\Plugins\Doodle -VS2019
"D:\Program Files\Epic Games\UE_5.0\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=E:\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=E:\Doodle\script\uePlug\5.0\Plugins\Doodle
"D:\Program Files\Epic Games\UE_5.1\Engine\Build\BatchFiles\RunUAT.bat" BuildPlugin -Plugin=E:\Doodle\script\uePlug\Doodle\Doodle.uplugin -TargetPlatforms=Win64 -Package=E:\Doodle\script\uePlug\5.1\Plugins\Doodle

$env:VCPKG_DEFAULT_TRIPLET="x64-windows"
"$(SolutionDir)doodle_plug.uproject" -skipcompile -run=DoodleAssCreate -path="E:\Users\TD\Documents\Unreal_Projects\doodle_plug_dev_4.27\test_file\doodle_import_data_main.json"
GenerateProjectFiles.bat -projectfiles -vscode -project=F:\doodle_plug_dev_4.27\doodle_plug.uproject -game -engine
]==]
