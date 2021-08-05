cmake_minimum_required(VERSION 3.20)

project(Doodle
        VERSION 2.7.32.2
        DESCRIPTION "动画制作中的一些辅助工具"
        LANGUAGES CXX
        HOMEPAGE_URL https://github.com/uuiid/Doodle)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
#设置qt自动参数
#set(CMAKE_AUTOUIC ON)
#set(CMAKE_AUTOMOC ON)
#set(CMAKE_AUTORCC ON)


add_compile_definitions(QT_NO_KEYWORDS UNICODE _UNICODE NOMINMAX)
#add_compile_options(/W4)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
#添加模块文件
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake")


#使用中文
add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")

#添加选项
option(RUN_QT_TOOL "run qt windeployqt" OFF)
option(RUN_fixup "run fixup " OFF)
option(RUN_COPY "run copy tools" OFF)


#配置版本文件
configure_file(DoodleConfig.h.in ${CMAKE_BINARY_DIR}/configure/DoodleConfig.h)
include_directories(${CMAKE_BINARY_DIR}/configure)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# #添加qt路径
# list(APPEND CMAKE_PREFIX_PATH F:/Qt/5.12.9/msvc2017_64)

#首先搜索我们的自定义函数
list(APPEND CMAKE_PREFIX_PATH ${PROJECT_SOURCE_DIR}/CMake)
include(CMakeRC)

#寻找一些外部库
#    find_package(unofficial-nana CONFIG REQUIRED)
#    target_link_libraries(main PRIVATE unofficial::nana::nana)
#
#find_package(Qt5 CONFIG COMPONENTS Core Widgets Gui LinguistTools  REQUIRED)
#GUI 库
find_package(wxWidgets REQUIRED)
find_package(unofficial-nana CONFIG REQUIRED)

find_package(Boost 1.75 REQUIRED COMPONENTS filesystem date_time log locale iostreams)
find_package(EnTT CONFIG REQUIRED)
# find_package(CURL REQUIRED)
find_package(Sqlpp11 REQUIRED)
find_package(GTest CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)
#target_link_libraries(main PRIVATE Catch2::Catch2)
find_package(FFMPEG REQUIRED)
#find_package(rttr CONFIG REQUIRED)
find_library(DOODLE_MAR mpr.lib REQUIRED)
# find_package(pugixml CONFIG REQUIRED)

find_package(Doxygen REQUIRED)

# 寻找opencv
find_package(OpenCV CONFIG REQUIRED)

#这两个是我自己写的寻找库的方法
#find_package(SqlppSqlite REQUIRED)
find_package(SqlppMySql REQUIRED)

#仅头库
find_package(magic_enum CONFIG REQUIRED)
#find_package(jsoncpp CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(date CONFIG REQUIRED)
find_package(cereal CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)
find_package(cryptopp CONFIG REQUIRED)
#服务器库
#应该会用其中一个
#find_package(cppzmq CONFIG REQUIRED)

#find_package(cppnetlib CONFIG REQUIRED)
#find_package(civetweb CONFIG REQUIRED)

#google序列化库
# find_package(protobuf CONFIG REQUIRED)
# target_link_libraries(main 
# PRIVATE protobuf::libprotoc 
# protobuf::libprotobuf
# protobuf::libprotobuf-lite)
#Google rpc库
#find_package(gRPC CONFIG REQUIRED)
## Note: 8 target(s) were omitted.
#target_link_libraries(main PRIVATE gRPC::gpr gRPC::upb gRPC::grpc gRPC::grpc++)
#
#find_package(modules CONFIG REQUIRED)
#target_link_libraries(main PRIVATE re2::re2 c-ares::cares)
find_package(gRPC CONFIG REQUIRED)
#target_link_libraries(main PRIVATE gRPC::gpr gRPC::grpc gRPC::grpc++ gRPC::grpc++_alts)
#target_link_libraries(main PRIVATE re2::re2 c-ares::cares)

find_package(Autodesk_FBX 2020 REQUIRED)
#搜索qt 安装辅助程序
# find_program(WINQT qt5::windeployqt REQUIRED)

#下载自定义嵌入文件
include(ExternalProject)
include(FetchContent)

# 添加树项目
# FetchContent_Declare(
#         st_tree
#         GIT_REPOSITORY https://github.com/erikerlandson/st_tree.git
#         GIT_TAG develop
# )
# FetchContent_GetProperties(st_tree)
# if (NOT st_tree_POPULATED)
#     FetchContent_Populate(st_tree)
#     execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${st_tree_SOURCE_DIR}/include ${PROJECT_SOURCE_DIR}/external/st_tree/)
# endif ()
# include_directories(${PROJECT_SOURCE_DIR}/external/st_tree/)
# 添加csv解析器
add_subdirectory(${PROJECT_SOURCE_DIR}/external/csv-parser)

#添加windows时区数据库
#FetchContent_Declare(
#        windowsZones
#        URL https://raw.githubusercontent.com/unicode-org/cldr/master/common/supplemental/windowsZones.xml
#        DOWNLOAD_NO_EXTRACT TRUE
#)
#FetchContent_Populate(windowsZones)
#execute_process(COMMAND ${CMAKE_COMMAND} -E copy_if_different ${windowsZones_SOURCE_DIR}/windowsZones.xml ${CMAKE_BINARY_DIR}/bin/tzdata/windowsZones.xml)
if (NOT EXISTS ${PROJECT_SOURCE_DIR}/external/tzdata/windowsZones.xml)
    file(DOWNLOAD
            https://raw.githubusercontent.com/unicode-org/cldr/master/common/supplemental/windowsZones.xml
            ${PROJECT_SOURCE_DIR}/external/tzdata/windowsZones.xml
            SHOW_PROGRESS)
    file(DOWNLOAD
            https://www.iana.org/time-zones/repository/tzdata-latest.tar.gz
            ${PROJECT_SOURCE_DIR}/external/tzdata/tzdata-latest.tar.gz
            SHOW_PROGRESS)
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xfz ${PROJECT_SOURCE_DIR}/external/tzdata/tzdata-latest.tar.gz
            WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/external/tzdata/
            RESULT_VARIABLE rv)
    file(REMOVE ${PROJECT_SOURCE_DIR}/external/tzdata/tzdata-latest.tar.gz)
endif ()
add_custom_target(
        tzdata_file
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/external/tzdata/ ${CMAKE_BINARY_DIR}/bin/tzdata/
)
add_custom_command(
        OUTPUT ${PROJECT_SOURCE_DIR}/doc/css/doxygen-awesome.css
        ${PROJECT_SOURCE_DIR}/CMake/CMakeRC.cmake
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/external/doxygen-awesome-css/doxygen-awesome.css ${PROJECT_SOURCE_DIR}/doc/css
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${PROJECT_SOURCE_DIR}/external/cmrc/CMakeRC.cmake ${PROJECT_SOURCE_DIR}/CMake/
        DEPENDS ${PROJECT_SOURCE_DIR}/external/doxygen-awesome-css/doxygen-awesome.css
        ${PROJECT_SOURCE_DIR}/external/cmrc/CMakeRC.cmake
)

#添加文档生成
set(DOXYGEN_USE_MDFILE_AS_MAINPAGE README.md)
set(DOXYGEN_WARNINGS NO)
set(DOXYGEN_WARN_IF_UNDOCUMENTED NO)
set(DOXYGEN_WARN_IF_DOC_ERROR NO)
set(DOXYGEN_OUTPUT_LANGUAGE Chinese)
set(DOXYGEN_IMAGE_PATH ${PROJECT_SOURCE_DIR}/doc)
set(DOXYGEN_EXCLUDE_PATTERNS
        */.idea
        */.vs
        */b_Ninja_VS*
        */cmake-build*
        */venv)
set(DOXYGEN_GENERATE_TREEVIEW YES)
set(DOXYGEN_HTML_EXTRA_STYLESHEET ${PROJECT_SOURCE_DIR}/doc/css/doxygen-awesome.css)
doxygen_add_docs(Doxygen
        ${PROJECT_SOURCE_DIR}/src
        ${PROJECT_SOURCE_DIR}/doc
        )

add_subdirectory(src)


enable_testing()
add_subdirectory(test)
add_subdirectory(tools)
#=====================安装打包过程===============================
# list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake)
# include(mypack)


#====================一些非qt组件的搜索和安装=============

if (RUN_fixup)
    #第一种搜索方式
    set(CMAKE_INSTALL_UCRT_LIBRARIES "C:/Program Files (x86)/Windows Kits/10/Redist/ucrt/DLLs/x64")
    include(InstallRequiredSystemLibraries)
endif (RUN_fixup)

list(APPEND DIRS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/bin)


if (RUN_COPY)
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
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/maya
            DESTINATION plug)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/script/uePlug
            DESTINATION plug)
endif (RUN_COPY)


if (RUN_fixup)
    #最后实现camke修复
    install(CODE "
        include(BundleUtilities)
        fixup_bundle(\"$<TARGET_FILE:DoodleExe>\"   \"\"   \"${DIRS}\")
        ")
endif (RUN_fixup)


#install(DIRECTORY ${CMAKE_BINARY_DIR}/plug/ DESTINATION plug)
install(DIRECTORY ${CMAKE_BINARY_DIR}/bin/ DESTINATION bin
        #PATTERN "*.manifest" EXCLUDE
        )


set(CPACK_GENERATOR "WIX")
set(CPACK_PACKAGE_EXECUTABLES DoodleExe;Doodle)
set(CPACK_CREATE_DESKTOP_LINKS DoodleExe)
set(CPACK_PACKAGE_VENDOR uuiid)
set(CPACK_PACKAGE_INSTALL_DIRECTORY uuiid)
set(CPACK_WIX_UPGRADE_GUID 96068D94-08C8-412B-9292-6DE8453E61B3)
# set(CPACK_WIX_PRODUCT_ICON ${CMAKE_CURRENT_LIST_DIR}/resource/inco/inco128_128.png)
# set(CPACK_PACKAGE_ICON ${CMAKE_CURRENT_LIST_DIR}/resource/inco/inco128_128.png)
# set(CPACK_WIX_CULTURES zh_CN)
include(CPack REQUIRED)

add_custom_target(cpack_${PROJECT_NAME}
        COMMAND ${CMAKE_CPACK_COMMAND} --build ${PROJECT_BINARY_DIR} --target package)
