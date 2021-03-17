cmake_minimum_required(VERSION 3.14)

project(Doodle
        VERSION 2.3.1.9
        DESCRIPTION "动画制作中的一些辅助工具"
        LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#设置qt自动参数
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)


#设置不使用qt_key
add_definitions(-DQT_NO_KEYWORDS)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#添加模块文件
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")


#使用中文
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

#添加选项
option(RUN_QT_TOOL "run qt windeployqt" OFF)
option(RUN_fixup   "run fixup "         OFF)
option(RUN_COPY    "run copy tools"     OFF)


#配置版本文件
configure_file(DoodleConfig.h.in ${CMAKE_BINARY_DIR}/configure/DoodleConfig.h)
include_directories(${CMAKE_BINARY_DIR}/configure)

# #添加qt路径
# list(APPEND CMAKE_PREFIX_PATH F:/Qt/5.12.9/msvc2017_64)

#首先搜索我们的自定义函数
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/CMake)

#寻找一些外部库
find_package(Qt5 CONFIG COMPONENTS Core Widgets Gui LinguistTools  REQUIRED)
find_package(Boost 1.75 REQUIRED COMPONENTS filesystem date_time regex iostreams log locale)
# find_package(CURL REQUIRED)
find_package(Sqlpp11 REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(FFMPEG  REQUIRED )
find_package(rttr CONFIG REQUIRED)
# find_package(pugixml CONFIG REQUIRED)

find_package(Doxygen REQUIRED)

# 寻找opencv
find_package(OpenCV CONFIG REQUIRED)

#这两个是我自己写的寻找库的方法
find_package(SqlppSqlite REQUIRED)
# find_package(SqlppMySql REQUIRED)

#仅头库
find_package(magic_enum CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(date CONFIG REQUIRED)

#服务器库
#应该会用其中一个
# find_package(cppzmq CONFIG REQUIRED)

# find_package(cppnetlib CONFIG REQUIRED)
# find_package(civetweb CONFIG REQUIRED)

#google序列化库
# find_package(protobuf CONFIG REQUIRED)
# target_link_libraries(main 
# PRIVATE protobuf::libprotoc 
# protobuf::libprotobuf
# protobuf::libprotobuf-lite)
#Google rpc库
# find_package(gRPC CONFIG REQUIRED)
#target_link_libraries(main PRIVATE gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts)
#target_link_libraries(main PRIVATE re2::re2 c-ares::cares)

find_package(Autodesk_FBX 2020 REQUIRED)
#搜索qt 安装辅助程序
# find_program(WINQT qt5::windeployqt REQUIRED)

#下载自定义嵌入文件
include(ExternalProject)
ExternalProject_Add(bin2cpp_prj 
        URL https://github.com/end2endzone/bin2cpp/releases/download/2.3.0/bin2cpp-2.3.0-win32.zip
        CONFIGURE_COMMAND ""
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        TEST_COMMAND ""
)
ExternalProject_Get_property(bin2cpp_prj SOURCE_DIR)
# message("Source dir of bin2cpp_prj = ${SOURCE_DIR}")
set(BIN2CPP ${SOURCE_DIR}/bin/bin2cpp.exe)

#添加文档生成
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
set(DOXYGEN_WARNINGS NO)
set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
set(DOXYGEN_WARN_IF_DOC_ERROR NO)
set(DOXYGEN_OUTPUT_LANGUAGE Chinese)
set(DOXYGEN_EXCLUDE_PATTERNS 
*/.idea
*/.vs
*/b_Ninja_VS*
*/venv)
doxygen_add_docs(Doxygen
${PROJECT_SOURCE_DIR}
)


add_subdirectory(src)

enable_testing()
add_subdirectory(test)
#=====================安装打包过程===============================
# list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
# include(mypack)


#====================一些非qt组件的搜索和安装=============

if(RUN_fixup)
        #第一种搜索方式
        set(CMAKE_INSTALL_UCRT_LIBRARIES "C:/Program Files (x86)/Windows Kits/10/Redist/ucrt/DLLs/x64")
        include(InstallRequiredSystemLibraries)
endif(RUN_fixup)

foreach(_var in LISTS ${CMAKE_FIND_ROOT_PATH})
     list(APPEND DIRS ${_var}/bin)
endforeach(_var)


if(RUN_COPY)
        # #复制资源
        # install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/
        # DESTINATION resource)
        # #复制tool
        # install(DIRECTORY ${CMAKE_SOURCE_DIR}/tools/
        # DESTINATION tools)
        # #复制我自己写的一些工具
        # install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/Cmd_tool
        # DESTINATION tools)
        # #复制插件
        # install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/maya_plug
        # DESTINATION plug)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug
        DESTINATION plug) 
endif(RUN_COPY)


if(RUN_fixup)
        #最后实现camke修复
        install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"$<TARGET_FILE:doodle>\"   \"\"   \"${DIRS}\")
        ")
endif(RUN_fixup)


# install(DIRECTORY ${CMAKE_BINARY_DIR}/plug/ DESTINATION plug)
# install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/ DESTINATION bin
# PATTERN "*.manifest" EXCLUDE )


set(CPACK_GENERATOR "WIX")

include(CPack REQUIRED)
