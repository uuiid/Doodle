cmake_minimum_required(VERSION 3.14)

project(Doodle LANGUAGES CXX)

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
set(Doodle_VERSION_MAJOR 2)
set(Doodle_VERSION_MINOR 2) 
set(Doodle_VERSION_REVIS 22)
configure_file(DoodleConfig.h.in ${CMAKE_BINARY_DIR}/configure/DoodleConfig.h)
configure_file(anzhuang.iss.in ${CMAKE_INSTALL_PREFIX}/anzhuang.iss)
include_directories(${CMAKE_BINARY_DIR}/configure)

#添加qt路径
list(APPEND CMAKE_PREFIX_PATH $ENV{QT_MYROOT}/5.12.9/msvc2017_64)

#首先搜索我们的自定义函数
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/CMake)

#寻找一些外部库
find_package(Qt5 CONFIG COMPONENTS Core Widgets Sql Test QuickControls2 LinguistTools REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem date_time regex iostreams log locale)
find_package(CURL REQUIRED)
find_package(Sqlpp11 REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(FFMPEG  REQUIRED )
find_package(rttr CONFIG REQUIRED)
find_package(pugixml CONFIG REQUIRED)

#这两个是我自己写的寻找库的方法
find_package(SqlppSqlite REQUIRED)
find_package(SqlppMySql REQUIRED)

#仅头库
find_package(magic_enum CONFIG REQUIRED)
find_package(jsoncpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(date CONFIG REQUIRED)

#服务器库
#应该会用其中一个
find_package(cppzmq CONFIG REQUIRED)

find_package(cppnetlib CONFIG REQUIRED)
find_package(civetweb CONFIG REQUIRED)

#google序列化库
find_package(protobuf CONFIG REQUIRED)
# target_link_libraries(main 
# PRIVATE protobuf::libprotoc 
# protobuf::libprotobuf
# protobuf::libprotobuf-lite)
#Google rpc库
find_package(gRPC CONFIG REQUIRED)
#target_link_libraries(main PRIVATE gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts)
#target_link_libraries(main PRIVATE re2::re2 c-ares::cares)


#搜索qt 安装辅助程序
find_program(WINQT windeployqt REQUIRED)

add_subdirectory(logger)
add_subdirectory(fileSystem)
#add_subdirectory(fileSyn)
add_subdirectory(pinyin)
add_subdirectory(core)
add_subdirectory(doodle_GUI)
add_subdirectory(doodle_server)


enable_testing()
add_subdirectory(test)
#=====================安装打包过程===============================
# list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
include(mypack)


#====================一些非qt组件的搜索和安装=============

if(RUN_fixup)
        #第一种搜索方式
        set(CMAKE_INSTALL_UCRT_LIBRARIES "C:/Program Files (x86)/Windows Kits/10/Redist/ucrt/DLLs/x64")
        include(InstallRequiredSystemLibraries)
endif(RUN_fixup)



#第二中验证加搜索方式
set(DOODLEAPPS ${CMAKE_INSTALL_PREFIX}/bin/doodle.exe)  # paths to executables
set(TESTAPPS ${CMAKE_INSTALL_PREFIX}/bin/test_my.exe)

get_filename_component(fixdir ${WINQT} DIRECTORY)

list(APPEND DIRS ${fixdir})
list(APPEND DIRS ${CMAKE_INSTALL_PREFIX}/lib)
foreach(_var in LISTS ${CMAKE_FIND_ROOT_PATH})
     list(APPEND DIRS ${_var}/bin)
endforeach(_var)


if(RUN_COPY)
        #复制资源
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/resource/
        DESTINATION resource)
        #复制tool
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/tools/
        DESTINATION tools)
        #复制我自己写的一些工具
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/Cmd_tool
        DESTINATION tools)
        #复制插件
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/maya_plug
        DESTINATION plug)
        install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug
        DESTINATION plug) 
endif(RUN_COPY)


if(RUN_fixup)
        #最后实现camke修复
        install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"${DOODLEAPPS}\"   \"\"   \"${DIRS}\")
        ")
        if(CMAKE_BUILD_TYPE EQUAL "DEBUG")
                install(CODE "
                include(BundleUtilities)
                fixup_bundle(\"${TESTAPPS}\"   \"\"   \"${DIRS}\")
                ") 
        endif(CMAKE_BUILD_TYPE EQUAL "DEBUG")
endif(RUN_fixup)



# include(CPack)
# ${projectDir}\out\install\${name}