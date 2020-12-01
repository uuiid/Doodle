cmake_minimum_required(VERSION 3.14)

project(Doodle_tools LANGUAGES CXX)

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
option(RUN_fixup   "run fixup "         OFF)

#添加qt路径
list(APPEND CMAKE_PREFIX_PATH $ENV{QT_MYROOT}/5.12.9/msvc2017_64)

find_package(Qt5 CONFIG COMPONENTS Core Widgets Test LinguistTools REQUIRED)
find_package(Boost COMPONENTS filesystem REQUIRED)

#搜索qt 安装辅助程序
find_program(WINQT windeployqt REQUIRED)

add_executable(export_abc WIN32
        src/sourePy.cpp
        src/main.h
        src/main.cpp
        )
target_include_directories(export_abc PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
target_link_libraries(export_abc Qt5::Core)
target_link_libraries(export_abc Qt5::Widgets)
target_link_libraries(export_abc ${Boost_LIBRARIES})
target_include_directories(export_abc PRIVATE ${Boost_INCLUDE_DIR})
target_compile_definitions(export_abc PUBLIC _WIN32_WINNT=0x0A00)

#安装基本组件
install(TARGETS export_abc
        DESTINATION bin)
#安装调试文件
install(FILES $<TARGET_PDB_FILE:export_abc>
        DESTINATION bin
        CONFIGURATIONS Debug)

if(RUN_fixup)
        #第一种搜索方式
        set(CMAKE_INSTALL_UCRT_LIBRARIES "C:/Program Files (x86)/Windows Kits/10/Redist/ucrt/DLLs/x64")
        include(InstallRequiredSystemLibraries)
endif(RUN_fixup)

get_filename_component(fixdir ${WINQT} DIRECTORY)

        
if(RUN_QT_TOOL)
        #运行qt
        install(CODE "
        execute_process(
        COMMAND ${WINQT} --compiler-runtime --dir ${CMAKE_INSTALL_PREFIX}/bin $<TARGET_FILE:export_abc>
        )
        ")  
endif(RUN_QT_TOOL)