cmake_minimum_required(VERSION 3.14)

project(Doodle LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#使用中文
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

#添加选项
option(RUN_QT_TOOL "run qt windeployqt" OFF)
option(RUN_fixup   "run fixup " OFF)
option(RUN_COPY    "run copy tools" OFF)


#启用测试
configure_file(DoodleConfig.h.in ${CMAKE_CURRENT_LIST_DIR}/DoodleConfig.h)

list(APPEND CMAKE_PREFIX_PATH $ENV{QT_MYROOT}/5.12.9/msvc2017_64)

find_package(Qt5 CONFIG COMPONENTS Core Widgets Test LinguistTools REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem date_time)
find_package(CURL REQUIRED)


add_subdirectory(logger)
add_subdirectory(fileSystem)
#add_subdirectory(fileSyn)
add_subdirectory(pinyin)
add_subdirectory(core)
add_subdirectory(doodle_GUI)


#添加测试
find_package(GTest CONFIG REQUIRED)

#enable_testing()
add_subdirectory(test)
#=====================安装打包过程===============================
#首先搜索我们的自定义函数
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
include(mypack)
#搜索qt 安装辅助程序
find_program(WINQT windeployqt REQUIRED)
#添加qt 安装辅助程序的一些命令

#====================一些非qt组件的搜索和安装=============
#第一种搜索方式
include(InstallRequiredSystemLibraries)
#第二中验证加搜索方式
set(DOODLEAPPS ${CMAKE_INSTALL_PREFIX}/bin/doodle.exe)  # paths to executables
set(TESTAPPS ${CMAKE_INSTALL_PREFIX}/bin/test_my.exe)

get_filename_component(fixdir ${WINQT} DIRECTORY)

list(APPEND DIRS ${fixdir})
list(APPEND DIRS ${CMAKE_INSTALL_PREFIX}/lib)
foreach(_var in LISTS ${CMAKE_FIND_ROOT_PATH})
     list(APPEND DIRS ${_var}/bin)
endforeach(_var)
list(APPEND DIRS "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/")
list(APPEND DIRS "C:/Program Files (x86)/Microsoft Visual Studio/2019/Community/Common7/IDE/Remote Debugger/x64/")

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